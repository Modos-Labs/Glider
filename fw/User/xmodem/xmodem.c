/***********************************************************************************************************************
 * XMODEM implementation with YMODEM support
 ***********************************************************************************************************************
 * Copyright 2001-2019 Georges Menie (www.menie.org)
 * Modified by Thuffir in 2019
 * Modified by Analog Devices in 2024
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of California, Berkeley nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **********************************************************************************************************************/

/*
 * This code has been modified by Analog Devices, Inc.
 */


/* this code needs standard functions memcpy() and memset()
   and input/output functions _inbyte() and _outbyte().

   the prototypes of the input/output functions are:
     int _inbyte(unsigned short timeout, void *); // timeout
     void _outbyte(int c, void *);

 */

/* Needed for memcpy() and memeset() */
#include <string.h>
#include "xmodem.h"

/***********************************************************************************************************************
 * Character constants
 **********************************************************************************************************************/
#define SOH   0x01
#define STX   0x02
#define EOT   0x04
#define ACK   0x06
#define NAK   0x15
#define CAN   0x18
#define CTRLZ 0x1A

#ifdef XMODEM_1K
/* 1024 for XModem 1k + 3 head chars + 2 crc */
#define XBUF_SIZE (1024 + 3 + 2)
#else
/* 128 for XModem + 3 head chars + 2 crc */
#define XBUF_SIZE (128 + 3 + 2)
#endif

#ifndef HAVE_CRC16
/***********************************************************************************************************************
 * Calculate the CCITT-CRC-16 value of a given buffer
 **********************************************************************************************************************/
static unsigned short crc16_ccitt(
  /* Pointer to the byte buffer */
  const unsigned char *buffer,
  /* length of the byte buffer */
  int length)
{
  unsigned short crc16 = 0;
  while(length != 0) {
    crc16  = (unsigned char)(crc16 >> 8) | (crc16 << 8);
    crc16 ^= *buffer;
    crc16 ^= (unsigned char)(crc16 & 0xff) >> 4;
    crc16 ^= (crc16 << 8) << 4;
    crc16 ^= ((crc16 & 0xff) << 4) << 1;
    buffer++;
    length--;
  }

  return crc16;
}
#endif

/***********************************************************************************************************************
 * Check block
 **********************************************************************************************************************/
static int check(int crc, const unsigned char *buf, int sz)
{
  if (crc) {
    unsigned short crc = crc16_ccitt(buf, sz);
    unsigned short tcrc = (buf[sz]<<8)+buf[sz+1];
    if (crc == tcrc)
      return 1;
  }
  else {
    int i;
    unsigned char cks = 0;
    for (i = 0; i < sz; ++i) {
      cks += buf[i];
    }
    if (cks == buf[sz])
      return 1;
  }

  return 0;
}

/***********************************************************************************************************************
 * Flush input
 **********************************************************************************************************************/
static void flushinput(XmodemInByte _inbyte, void *ctx)
{
  while (_inbyte(((DLY_1S)*3)>>1, ctx) >= 0)
    ;
}

/***********************************************************************************************************************
 * XMODEM Receive
 **********************************************************************************************************************/
int XmodemReceive(StoreChunkType storeChunk, void *ctx, int destsz, int crc, int mode, XmodemInByte _inbyte, XmodemOutByte _outbyte)
{
  unsigned char xbuff[XBUF_SIZE];
  unsigned char *p;
  int bufsz;
  unsigned char trychar = (crc == 2) ? 'G' : (crc ? 'C' : NAK);
  unsigned char packetno = mode ? 0 : 1;
  int i, c, len = 0;
  int retry, retrans = MAXRETRANS;
  int sync_retry = MAXSYNCTRETRY;

  for(;;) {
    for( retry = 0; retry < sync_retry; ++retry) {
      if (trychar) _outbyte(trychar, ctx);
      if ((c = _inbyte((DLY_1S)<<1, ctx)) >= 0) {
        switch (c) {
          case SOH:
            bufsz = 128;
            goto start_recv;
#ifdef XMODEM_1K
          case STX:
            bufsz = 1024;
            goto start_recv;
#endif
          case EOT:
            if(storeChunk) {
              storeChunk(ctx, NULL, 0);
            }
            _outbyte(ACK, ctx);
            return len; /* normal end */
          case CAN:
            if ((c = _inbyte(DLY_1S, ctx)) == CAN) {
              flushinput(_inbyte, ctx);
              _outbyte(ACK, ctx);
              return -1; /* canceled by remote */
            }
            break;
          default:
            break;
        }
      }
    }
    if (trychar == 'G') { trychar = 'C'; crc = 1; continue; }
    //else if (trychar == 'C') { trychar = NAK; crc = 0; sync_retry = 2; continue; }
    flushinput(_inbyte, ctx);
    _outbyte(CAN, ctx);
    _outbyte(CAN, ctx);
    _outbyte(CAN, ctx);
    return -2; /* sync error */

    start_recv:
    trychar = 0;
    p = xbuff;
    *p++ = c;
    for (i = 0;  i < (bufsz+(crc?1:0)+3); ++i) {
      if ((c = _inbyte(DLY_1S, ctx)) < 0) goto reject;
      *p++ = c;
    }

    if (xbuff[1] == (unsigned char)(~xbuff[2]) &&
        (xbuff[1] == packetno || xbuff[1] == (unsigned char)packetno-1) &&
        check(crc, &xbuff[3], bufsz)) {
      if (xbuff[1] == packetno) {
        register int count = destsz - len;
        if (count > bufsz) count = bufsz;
        if (count > 0) {
          if(storeChunk) {
            storeChunk(ctx, &xbuff[3], count);
          }
          else {
            memcpy (&((unsigned char *)ctx)[len], &xbuff[3], count);
          }
          len += count;
        }
        ++packetno;
        retrans = MAXRETRANS+1;
      }
      if (--retrans <= 0) {
        flushinput(_inbyte, ctx);
        _outbyte(CAN, ctx);
        _outbyte(CAN, ctx);
        _outbyte(CAN, ctx);
        return -3; /* too many retry error */
      }
      if(crc != 2) _outbyte(ACK, ctx);
      if(mode) return len; /* YMODEM control block received */
      continue;
    }
    reject:
    flushinput(_inbyte, ctx);
    _outbyte(NAK, ctx);
  }
}

/***********************************************************************************************************************
 * XMODEM Transmit
 **********************************************************************************************************************/
int XmodemTransmit(FetchChunkType fetchChunk, void *ctx, int srcsz, int onek, int mode, XmodemInByte _inbyte, XmodemOutByte _outbyte)
{
  unsigned char xbuff[XBUF_SIZE];
  int bufsz, crc = -1;
  unsigned char packetno = mode ? 0 : 1;
  int i, c, len = 0;
  int retry;

  for(;;) {
    for( retry = 0; retry < MAXSYNCTRETRY; ++retry) {
      if ((c = _inbyte((DLY_1S)<<1, ctx)) >= 0) {
        switch (c) {
          case 'G':
            crc = 2;
            goto start_trans;
          case 'C':
            crc = 1;
            goto start_trans;
          case NAK:
            crc = 0;
            goto start_trans;
          case CAN:
            if ((c = _inbyte(DLY_1S, ctx)) == CAN) {
              _outbyte(ACK, ctx);
              flushinput(_inbyte, ctx);
              return -1; /* canceled by remote */
            }
            break;
          default:
            break;
        }
      }
    }
    _outbyte(CAN, ctx);
    _outbyte(CAN, ctx);
    _outbyte(CAN, ctx);
    flushinput(_inbyte, ctx);
    return -2; /* no sync */

    for(;;) {
      start_trans:
      c = srcsz - len;
#ifdef XMODEM_1K
      if(onek && (c > 128)) {
        xbuff[0] = STX; bufsz = 1024;
      }
      else
#endif
      {
        xbuff[0] = SOH; bufsz = 128;
      }
      xbuff[1] = packetno;
      xbuff[2] = ~packetno;
      if (c > bufsz) c = bufsz;
      if (c > 0) {
        memset (&xbuff[3], mode ? 0 : CTRLZ, bufsz);
        if(fetchChunk) {
          fetchChunk(ctx, &xbuff[3], c);
        }
        else {
          memcpy (&xbuff[3], &((unsigned char *)ctx)[len], c);
        }
        if (crc) {
          unsigned short ccrc = crc16_ccitt(&xbuff[3], bufsz);
          xbuff[bufsz+3] = (ccrc>>8) & 0xFF;
          xbuff[bufsz+4] = ccrc & 0xFF;
        }
        else {
          unsigned char ccks = 0;
          for (i = 3; i < bufsz+3; ++i) {
            ccks += xbuff[i];
          }
          xbuff[bufsz+3] = ccks;
        }
        for (retry = 0; retry < MAXRETRANS; ++retry) {
          for (i = 0; i < bufsz+4+(crc?1:0); ++i) {
            _outbyte(xbuff[i], ctx);
          }
          c = (crc == 2) ? ACK : _inbyte(DLY_1S, ctx);
          if (c >= 0 ) {
            switch (c) {
              case ACK:
                ++packetno;
                len += bufsz;
                goto start_trans;
              case CAN:
                if ((c = _inbyte(DLY_1S, ctx)) == CAN) {
                  _outbyte(ACK, ctx);
                  flushinput(_inbyte, ctx);
                  return -1; /* canceled by remote */
                }
                break;
              case NAK:
              default:
                break;
            }
          }
        }
        _outbyte(CAN, ctx);
        _outbyte(CAN, ctx);
        _outbyte(CAN, ctx);
        flushinput(_inbyte, ctx);
        return -4; /* xmit error */
      }
      else if(mode) {
        return len; /* YMODEM control block sent */
      }
      else {
        for (retry = 0; retry < 10; ++retry) {
          _outbyte(EOT, ctx);
          if ((c = _inbyte((DLY_1S)<<1, ctx)) == ACK) break;
        }
        if(c == ACK) {
          return len; /* Normal exit */
        }
        else {
          flushinput(_inbyte, ctx);
          return -5; /* No ACK after EOT */
        }
      }
    }
  }
}
