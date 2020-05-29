#!/usr/bin/python
import struct
import sys
import argparse
import os

__version__ = 1

def header():
	scd = [ # ...
		# struct item / length in word / default value
		["magic0",  1, 0xfacefeed],
		["magic1",  1, 0xdeadbeaf],
		["version",	1, __version__],
		["valid",   1, 0],
		["faddr",   1, 0x10000],
		["xip", 	1, 0],
		["size",    1, 0],

		["wbase",   1, 0],
		["hbase",   1, 0],
		["aeskey",  8, 0],
		["hkey",    8, 0],
		["rev",     31, 0],
		["hmac",    8, 0],
	]
	return scd

def set_header_field(scd, size, xip, faddr=0x10000):

	for i in (scd):
		if i[0] == "faddr":
			i[2] = faddr
		elif i[0] == "size": 
			i[2] = size
		elif i[0] == "xip": 
			i[2] = xip
	return scd
	

def dump_head_for_c(name, h):
	print
	print r"// scd header dump by %s version %d" % (name, __version__)
	print r"struct aai_scd_t { "
	for (name, length, v) in h: 
		if length == 1: 
			print r"    uint32_t %s;    // 0x%x" % (name, v)
		else:
			print r"    uint32_t %s[%d]; " % (name, length)
	print r"}; "
	print

def generate_image(fname, h, imaged):

	for i in range(len(h)):
		e = h[i]
		if i == 0: # ...
			scd_h = struct.pack("I", e[2])
		else:
			for j in range(e[1]):
				scd_h += struct.pack("I", e[2])

	if len(scd_h) != 256:
		print "!!scd length should be 256, but", len(scd_h)
		sys.exit(1)

	f = open(fname, "wb+")
	f.write(scd_h)
	f.write(imaged)
	f.close()
	print "scd header added to " + fname 

if __name__ == "__main__":
	
	# - optional
	parser = argparse.ArgumentParser()
	#parser.add_argument("-xip", help="use sxip or not", action="store_true", default=False)
	parser.add_argument("-dump", help="dump scd C header", action="store_true", default=False)
	parser.add_argument("fname", type=str, help="filename")
	parser.parse_args()
	args = parser.parse_args()

	if args.dump:
		h = header()
		dump_head_for_c(sys.argv[0], h)
		sys.exit(0)

	if os.path.isfile(args.fname):
		f = open(args.fname, "rb")
		fdata = f.read()
		f.close()
	else:
		print "!!file: " + args.fname + " not exist"
		sys.exit(1)

	if len(fdata) % 256:
		pad = 256 - len(fdata) % 256
		print "padding %d bytes" % pad
		fdata = fdata + "\x00" * pad

	# 0xfacefeed 0xdeadbeaf
	if (fdata[0]== '\xed' and fdata[1] == '\xfe' and fdata[2] == '\xce' and fdata[3] == '\xfa') \
		and (fdata[4]== '\xaf' and fdata[5] == '\xbe' and fdata[6] == '\xad' and fdata[7] == '\xde'):
		print "!!Seems aai aready has scd header, abort"
		sys.exit(1)

	h = header()

	hxip = set_header_field(h, len(fdata), 1)
	generate_image(args.fname+".xip", hxip, fdata)

	hnoxip = set_header_field(h, len(fdata), 0)
	generate_image(args.fname+".noxip", hnoxip, fdata)
