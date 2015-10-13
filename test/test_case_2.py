# The following modules are required: ctypes, os, math, random
from ctypes import *
import ctypes


import os, sys
import math
from random import *
import hashlib
from bats import Encoder, Decoder, NIODecoder, Recoder

import subprocess
def callback():
	#ret = subprocess.call(['diff', sys.argv[1], 'output'])
	#sys.exit(ret)
	pass


def cal_hash(f, block_size):
	while True:
		data = f.read(block_size)
		if not data:
			break
		hashlib.sha256().update(data)
	return hashlib.sha256().hexdigest()

enc = Encoder.fromfile(sys.argv[1])	
myfile = open(sys.argv[1], 'rb')

dec = NIODecoder.tofile('output', os.path.getsize(sys.argv[1]), callback, cal_hash(myfile, 512*1024))

myfile.close()

# Create recoder instance
rec = Recoder(16, 1024)

	
Rcnt = 1
Scnt = 1

# channel loss rate
r = 0.05
	
	#for i in range(30000):
	#	send = enc.genPacket()
	#print "Ended"
	#sys.exit(1)
	
	# Check whether decoding is complete. (Note: no callback feature at this moment yet)
import time
t0 = time.time()
	#for i in range(10240):
	#	enc.genPacket()
	#print "Ended...", time.time()-t0
	#sys.exit(1)
buf = []
while not dec.complete():
	send = enc.genPacket() # Generate a packet
	#Test for new batch
	if send == '':
		#print "New Batch!"
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
			#print "All packets in batch lost"
			pass
		continue
	#simulate channel
	if random() >= r:
		buf.append(send)
	else:
		pass
		Scnt += 1

