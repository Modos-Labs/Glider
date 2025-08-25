import configparser
import argparse
import csv
import os

def process_table(fn, newfn):
    table = {}
    with open(fn, newline='') as csvfile:
        csvreader = csv.reader(csvfile, delimiter=',')
        for row in csvreader:
            src = row[0]
            dst = row[1]
            seq = row[2:-1]
            table[f'{src}_{dst}'] = seq
    newtable = {}
    # We drop all odd numbered entries
    for src in range(16):
        for dst in range(16):
            seq = table[f'{src*2}_{dst*2}']
            newtable[f'{src}_{dst}'] = seq
    with open(newfn, 'w') as csvfile: 
        for src in range(16):
            for dst in range(16):
                ss = ','.join(newtable[f'{src}_{dst}'])
                csvfile.write(f'{src},{dst},{ss}\n')

def main():
    parser = argparse.ArgumentParser(prog='wvfm_converter')
    parser.add_argument('filename')
    args = parser.parse_args()
    config = configparser.ConfigParser()
    config.read(args.filename)
    #print(config.sections())

    version = config['WAVEFORM']['VERSION']
    if version != '2.0':
        raise Exception("Unsupported")
    prefix = config['WAVEFORM']['PREFIX']
    # name = config['WAVEFORM']['NAME']
    bpp = int(config['WAVEFORM']['BPP'])
    if bpp != 5:
        raise Exception("Input needs to be an 5bpp waveform")
    modes = int(config['WAVEFORM']['MODES'])
    temps = int(config['WAVEFORM']['TEMPS'])
    tables = int(config['WAVEFORM']['TABLES'])

    # Get all framecounts
    # fc = []
    # for i in range(tables):
    #     fc.append(config['WAVEFORM'][f'TB{i}FC'])

    # trange = []
    # for i in range(temps):
    #     trange.append(config['WAVEFORM'][f'T{i}RANGE'])

    dirname = os.path.dirname(os.path.abspath(args.filename))

    for i in range(tables):
        oldname = f'{prefix}_TB{i}.csv'
        newname = f'{prefix}_4bpp_TB{i}.csv'
        process_table(os.path.join(dirname, oldname),
                      os.path.join(dirname, newname))

    with open(os.path.join(dirname, f'{prefix}_4bpp_desc.iwf'), 'w') as fp:
        fp.write('[WAVEFORM]\n')
        fp.write('VERSION = 2.0\n')
        fp.write(f'PREFIX = {prefix}_4bpp\n')
        fp.write(f'NAME = {config['WAVEFORM']['NAME']}\n')
        fp.write(f'BPP = 4\n')
        fp.write(f'MODES = {modes}\n')
        fp.write(f'TEMPS = {temps}\n')
        fp.write(f'TABLES = {tables}\n')
        fp.write('\n')
        for i in range(temps):
            fp.write(f'T{i}RANGE = {config['WAVEFORM'][f'T{i}RANGE']}\n')
        fp.write('\n')
        for i in range(tables):
            fp.write(f'TB{i}FC = {config['WAVEFORM'][f'TB{i}FC']}\n')
        fp.write('\n')
        for i in range(modes):
            fp.write(f'[MODE{i}]\n')
            fp.write(f'NAME = {config[f'MODE{i}']['NAME']}\n')
            for j in range(temps):
                fp.write(f'T{j}TABLE = {config[f'MODE{i}'][f'T{j}TABLE']}\n')
            fp.write('\n')

if __name__ == "__main__":
    main()