all: mxc_wvfm_asm

mxc_wvfm_asm: main.c ini.c csv.c fread_csv_line.c split.c
	gcc -O1 -g main.c ini.c csv.c fread_csv_line.c split.c -o mxc_wvfm_asm

clean:
	rm -f mxc_wvfm_asm
