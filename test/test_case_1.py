#!/usr/bin/env python
import os
from random import random
import bats
from bats import Encoder, Decoder, Recoder

PACKET_SIZE = bats.PACKET_SIZE

if __name__ == "__main__":
	import sys

	if len(sys.argv) == 1:
		input_file = 'input'
	else:
		input_file = sys.argv[1]

	enc = Encoder.fromfile(input_file)
	
	# Create decoder instance given destination filename and filesize.
	dec = Decoder.tofile('output', os.path.getsize(input_file))
	
	# Create recoder instance
	rec = Recoder(16, PACKET_SIZE)

	
	Rcnt = 1
	Scnt = 1

	# channel loss rate
	r = 0.05
	
	import time
	t0 = time.time()
	buf = []
	while not dec.complete():
		send = enc.genPacket() # Generate a packet
		#Test for new batch
		if send == '':
			# Recode
			if len(buf) > 0:
				tmp = ''.join(buf)
				for i in range(0, 16):
					if random() >= r:
						resend = rec.genPacket(tmp, len(buf))
						dec.receivePacket(resend)
						Rcnt += 1
					else:
						pass
						#print "Packet lost in second link"
				# reset buffer
				del buf[0:len(buf)]
			else:
				pass
				#print "All packets in batch lost"
			continue
		#simulate channel
		if random() >= r:
			buf.append(send)
		else:
			pass
			#print "Packet lost in first link"
		Scnt += 1

	dec.logRankDist()

	import subprocess
	ret = subprocess.call(['diff', input_file, 'output'])
	#print "case_1: %d" % ret
	sys.exit(ret)
