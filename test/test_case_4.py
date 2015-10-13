import json
import base64
import os
from random import random
import common
from common import hashFile

import bats
from bats import NIODecoder
from bats import Decoder
from bats import Recoder, Encoder, PACKET_SIZE, BATCH_SIZE

def test_bats(from_file, to_file):
	def a():
		print 'a()'
	r = 0
	e = Encoder.fromfile(from_file)
	for i in xrange(10001):
		e.genPacket()
	#f = open("pp", "r+b")

	d = Decoder.tofile(to_file, os.path.getsize(from_file), a, hashFile(from_file))
	j = ''
	ib = 0
	h = {}
	hm = 0
	hid = {}
	hidm = 0

	while not d.complete():
		s = e.genPacket()
		#s = f.read(1042)
		if s == '':
			continue
		if s not in h:
			h[s] = 1
			hm = 1 if 1>hm else hm
		else:
			h[s] += 1
			hm = h[s] if h[s]>hm else hm
		i =  (ord(s[0])+ord(s[1])*256)
		if i not in hid:
			hid[i] = 1
			hidm = 1 if 1>hidm else hidm
		else:
			hid[i] += 1
			hidm = hid[i] if hid[i]>hidm else hidm
		if s:
			j = base64.b64decode(json.loads(json.dumps(base64.b64encode(s))))
			
			d.receivePacket(j)
			r += 1
			ib = i if i>ib else ib
			print ib, i, hm, hidm
	print type(j)
	print 'received:%d,decoded:%d'%(r, d.getDecoded())

def test_bats_s(from_file, to_file):
	def a():
		print 'a()'
	loss_rate = 0.1
	r = 0
	e = Encoder.fromfile(from_file)
	d = Decoder.tofile(to_file, os.path.getsize(from_file), a, hashFile(from_file))

	while not d.complete():
		s = e.genPacket()
		if s:
			if random() >= loss_rate:
				d.receivePacket(s)
				r += 1
	print 'received:%d,decoded:%d'%(r, d.getDecoded())

def test_bats_sr(from_file, to_file):
	def a():
		print 'a()'
	loss_rate = 0.1
	r = 0
	e = Encoder.fromfile(from_file)
	d = NIODecoder.tofile(to_file, os.path.getsize(from_file), a, hashFile(from_file))
	# d = Decoder.tofile(to_file, os.path.getsize(from_file))
	rec = Recoder(BATCH_SIZE, PACKET_SIZE)

	while not d.complete():
		s = e.genPacket()
		if s:
			if random() >= loss_rate:
				s = rec.genPacket(s,1)
				if random() >= loss_rate:
					d.receivePacket(s)
					r += 1
	print 'received:%d,decoded:%d'%(r, d.getDecoded())
if __name__ == "__main__":
	import sys
	test_bats_sr(sys.argv[1], sys.argv[2])
