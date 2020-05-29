#!/usr/bin/env python
import sys, getopt, array
import os
import collections
from collections import defaultdict, OrderedDict, namedtuple
import subprocess

#exec mode values: 0xFFFE:XIP, 0xFFFD:LE 0xFFFB:LAE 0x0:Invalid 0xFFFF:End

def usage():
    print
    print("Construct Toc binary from a group of SBIs. All entries are NONE")
    print("Usage: togen.py -hv -b <bin dir> -o <output file name> -e <binary file>")
    print
    print("Options:")
    print("\t-h                              Display this message")
    print("\t-v                              Verbose")
    print("\t-e <exec mode>                  Exec mode for the SBI image")
    print("\t-b <bin dir>                    Path to all bin files")
    print("\t-o <output file name>           Name of generated binary ToC image")
    print


SbiInfo = namedtuple('SbiInfo', 'file offset power_mode exec_mode')
TocInfo = namedtuple('TocInfo', 'toc_validity media_cmd_len media_cmd sbi_info')

media_type_size = 0
toc_header_size = 512
exec_mode_avail = 0
exec_mode = 0

execmode_cmdline = 0xAAAA

toc_info = [
    # TOC 1
    TocInfo(toc_validity = 0xAA55AA55, media_cmd_len = 0, media_cmd = [0x2, 0x24000000, 0, 0, 0x2, 0x24000004, 0, 0,0x2, 0x24000000, 0, 0, 0x2, 0x24000004, 0, 0],
            sbi_info = [
                SbiInfo(file = "sbi.bin", offset = 0x400, power_mode = 0, exec_mode = execmode_cmdline),
            ])
]

#parent directory
dir = os.path.dirname(os.path.dirname(os.getcwd()))
dir_toc = os.getcwd()
dir_bin = os.path.join(dir, 'BIN_images').strip()

ctable = [
	0x0, 0x77073096, 0xee0e612c, 0x990951ba,
	0x76dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
	0xedb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x9b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
	0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
	0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
	0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
	0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
	0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
	0x76dc4190, 0x1db7106, 0x98d220bc, 0xefd5102a,
	0x71b18589, 0x6b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0xf00f934, 0x9609a88e, 0xe10e9818,
	0x7f6a0dbb, 0x86d3d2d, 0x91646c97, 0xe6635c01,
	0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
	0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
	0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
	0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
	0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
	0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
	0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
	0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
	0xedb88320, 0x9abfb3b6, 0x3b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x4db2615, 0x73dc1683,
	0xe3630b12, 0x94643b84, 0xd6d6a3e, 0x7a6a5aa8,
	0xe40ecf0b, 0x9309ff9d, 0xa00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
	0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
	0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
	0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
	0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
	0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
	0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
	0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x26d930a,
	0x9c0906a9, 0xeb0e363f, 0x72076785, 0x5005713,
	0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0xcb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0xbdbdf21,
	0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
	0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
	0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
	0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
	0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
	0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
	0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
	0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
]

def calc_crc32(data_array):
    cval = 0xffffffff;
    i = 0

    for a in data_array:
        cval = (cval >> 8) ^ ctable[(cval & 0xFF) ^ a];

    return cval

def get_file_info(bin_files):
    s = []

    for file in bin_files:
        s.append(os.stat(file).st_size)

    return s

def string_to_number(str_num):

    try:
        return int(str_num, 0)
    except ValueError as e:
        error(str_num + ": ValueError")

def int_to_bytes(val, rev):
    if rev == True:
        return [int(hex(val >> i & 0xff), 0) for i in (0, 8, 16, 24)]
    else:
        return [int(hex(val >> i & 0xff), 0) for i in (24, 16, 8, 0)]

def short_to_bytes(val, rev):
    if rev == True:
        return [int(hex(val >> i & 0xff), 0) for i in (0, 8)]
    else:
        return [int(hex(val >> i & 0xff), 0) for i in (8, 0)]

def get_toc_header(i):
    global exec_mode_avail
    global exec_mode

    header = []
    
    header.extend(int_to_bytes(0x5A5A5A5A, False))
    header.extend(int_to_bytes(toc_info[i].toc_validity, True))
    header.extend(short_to_bytes(toc_info[i].media_cmd_len, True))
    header.extend(short_to_bytes(media_type_size, False))
    if toc_info[i].media_cmd_len != 0:
        j = 0
        while j < toc_info[i].media_cmd_len/4:
            header.extend(int_to_bytes(toc_info[i].media_cmd[j], True))
            j += 1
    
    j = 0
    while j < len(toc_info[i].sbi_info):
        if (exec_mode_avail == 1) & (execmode_cmdline == toc_info[i].sbi_info[j].exec_mode):
            header.extend(short_to_bytes(exec_mode, True))
        else:
            header.extend(short_to_bytes(toc_info[i].sbi_info[j].exec_mode, True))
        header.extend(short_to_bytes(toc_info[i].sbi_info[j].power_mode, True))
        header.extend(int_to_bytes(toc_info[i].sbi_info[j].offset, True))
        j += 1
    
    # ToC Tag + Toc Validity + Media-Type-Size + Media cmd len + media cmd + num_sbis*(power_mode + exec_mode + offset)
    offset = 4 + 4 + 2 + 2 + toc_info[i].media_cmd_len + len(toc_info[i].sbi_info)*(2 + 2 + 4);
    
    if offset > toc_header_size:
       print ("Error: Too many SBI images in ToC %d" % (i))
       sys.exit(2)

    padding = toc_header_size - offset
    if padding > 0:
        header.extend([0xFF] * padding)
    
    return bytearray(header)

def get_toc_content(i, current_offset, bin_dir):
    content = []
    sbi_count = len(toc_info[i].sbi_info)
    
    j = 0
    while j < sbi_count:
        # Check boot offset validity
        if current_offset > toc_info[i].sbi_info[j].offset:
            print ("Error: Boot location incorrect for TOC %d -> SBI %d" % (i, j))
            print ("       Given -> %d, Should be atleast %d" % (toc_info[i].sbi_info[j].offset, current_offset))
            sys.exit(2)

        # Prepend padding
        prepend = toc_info[i].sbi_info[j].offset - current_offset
        if prepend > 0:
            content.extend([0xFF] * prepend)
            current_offset += prepend

        file = os.path.join(bin_dir, toc_info[i].sbi_info[j].file).strip()
        try:
            fh = open(file, 'rb')
            file_data = fh.read()
            fh.close()
            content.extend(file_data)
            current_offset += len(file_data)
        except IOError as e:
            print("File:\"" + file + "\": I/O error({0}): {1}".format(e.errno, e.strerror))
            sys.exit(2)
        j += 1

    return bytearray(content)

def create_toc(output, bin_files, bin_file_sizes):
    header = []
    size = (len(bin_file_sizes) * 8) + 4
    current_offset = size + 12

    header.extend(int_to_bytes(0x5A5A5A5A, False))
    header.extend(short_to_bytes(len(bin_file_sizes), True))
    header.extend(short_to_bytes(size, True))

    i = 0
    for size in bin_file_sizes:
        print("Entry:%d offset:%d" % (i, current_offset))
        header.extend(short_to_bytes(0, False))
        header.extend(short_to_bytes(0, False))
        header.extend(int_to_bytes(current_offset, True))
        current_offset += size
        i += 1

    byte_array =  bytearray(header)
    crc = calc_crc32(byte_array)
    header = int_to_bytes(crc, True)
    header.extend(int_to_bytes(0x5555AAAA, True))
    byte_array.extend(bytearray(header))

    i = 0
    for file in bin_files:
        try:
            fh = open(file, 'rb')
            file_data = bytearray(fh.read())
            fh.close()
            byte_array.extend(file_data)
        except IOError as e:
            error("File:\"" + file + "\": I/O error({0}): {1}".format(e.errno, e.strerror))

    return byte_array

def show_array(byte_array):
    i = 0
    line = 0
    for byte in byte_array:
        if line == 0:
            print("%03d:" % (i)),
        line += 1
        print("%02x" % (byte)),

        if line == 32:
            line = 0
            print
        i += 1
    print

def main(argv):
    global exec_mode_avail
    global exec_mode

    out_file_name = ''
    verbose = False
    labels = []
    bin_dir = "."
    current_offset = 0

    try:
       opts, args = getopt.getopt(argv,"hvb:o:e:",["ofile="])
    except getopt.GetoptError:
       usage()
       sys.exit(2)
    for opt, arg in opts:
       if opt == '-h':
           usage()
           sys.exit()
       elif opt == '-v':
           verbose = True
       elif opt == '-b':
           bin_dir = arg
       elif opt == '-e':
           exec_mode = int(arg, 0)
           exec_mode_avail = 1;
       elif opt in ("-o", "--ofile"):
           out_file_name = arg

    if (len(out_file_name) == 0):
       usage()
       sys.exit(2)
    else:
        print("Output file: %s " % (out_file_name))

    try:
        output = open(out_file_name, 'wb')
        # Generate TOC headers first
        limit = min (2, len(toc_info))
        i = 0
        while i < limit:
            header = get_toc_header(i)
            output.write(header)
            current_offset += len(header)
            i += 1
        # Then write the SBI images
        i = 0
        while i < limit:
            content = get_toc_content(i, current_offset, bin_dir)
            output.write(content)
            current_offset += len(content)
            i += 1

        output.close()
    except IOError as e:
        error("File:\"" + out_file_name + "\": I/O error({0}): {1}".format(e.errno, e.strerror))
        
    sys.exit(0);

if __name__ == "__main__":
        main(sys.argv[1:])

