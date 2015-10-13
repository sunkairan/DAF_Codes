# The following modules are required: ctypes, os, math, random
from ctypes import *
import ctypes

import os, sys
import math
from random import *

platform = sys.platform
if "darwin" == platform:
	lib = cdll.LoadLibrary("libbatscore.dylib") 

elif platform.startswith("linux"):
	lib = cdll.LoadLibrary(os.getcwd() + '/' + "libbatscore.so") 

lib.bats_init()
BATCH_SIZE = 16
ORDER = 8
PACKET_SIZE = 1024


class Encoder(object):
	@classmethod
	def fromfile(cls, path):
		size = os.path.getsize(path)

		byte = open(path, 'rb').read()
		self = cls(BATCH_SIZE, int(math.ceil(size/float(PACKET_SIZE))), PACKET_SIZE, byte)
		self.s = byte
		return self

	def __init__(self, batchsize, pktnum, pktsize, input):
		lib.BatsEncoder_new.restype = c_void_p
		self.obj = c_void_p(lib.BatsEncoder_new(c_int(batchsize), 
							c_int(pktnum), 
							c_int(pktsize), 
							cast(input, POINTER(c_ubyte))))
		lib.BatsEncoder_selectDegree(self.obj)
		self.M = batchsize
		self.K = pktnum
		self.T = pktsize
		self.pkt_id_in_batch = -1

	def genPacket(self):
		#print 'Calling genPacket...'
		#emit the first stored packet in a new batch
		self.pkt_id_in_batch += 1
		#print 'pkt_id_in_batch: ', self.pkt_id_in_batch
		if self.pkt_id_in_batch == self.M:
			self.pkt_id_in_batch = -1
			return ''

                lib.BatsEncoder_genPacket.restype = c_void_p
                result = lib.BatsEncoder_genPacket(self.obj)
		send = ctypes.string_at(result, 2+self.M+self.T)
		return send

class Decoder(object):
	@classmethod
	def tofile(cls, path, size):
		self = cls(BATCH_SIZE, int(math.ceil(size/float(PACKET_SIZE))), PACKET_SIZE)
		self.filepath = path
		self.filesize = size
		return self
	def __init__(self, batchsize, pktnum, pktsize):

		self.M = batchsize
		self.K = pktnum
		self.T = pktsize

		self.buf = '\x00' * (self.K * self.T)
		lib.BatsDecoder_new.restype = c_void_p
		self.obj = c_void_p(lib.BatsDecoder_new(c_int(batchsize), 
							c_int(pktnum), 
							c_int(pktsize), 
							cast(self.buf, POINTER(c_ubyte))))

		lib.BatsDecoder_selectDegree(self.obj)
		#self.F = ff

	def receivePacket(self, recv):
		lib.BatsDecoder_receivePacket(self.obj, cast(recv, POINTER(c_ubyte)))
		# new section
		if self.complete():
			if hasattr(self, 'filepath'):
				#Bugfix: remove zero pad by internal attribute filesize
				open(self.filepath, 'wb').write(self.buf[:self.filesize]) 
				lib.BatsDecoder_logRankDist(self.obj)
			else:
				lib.BatsDecoder_logRankDist(self.obj)
				self.num_inact = lib.BatsDecoder_numInact(self.obj)


	def complete(self):
		lib.BatsDecoder_complete.restype = c_bool
		return lib.BatsDecoder_complete(self.obj, c_double(1.0))

	def getDecoded(self):
		return lib.BatsDecoder_getDecoded(self.obj)

	def logRankDist(self):
		lib.BatsDecoder_logRankDist(self.obj)

class Recoder(object):
	def __init__(self, batchsize=BATCH_SIZE, pktsize=PACKET_SIZE):
		lib.NCCoder_new.restype = c_void_p
		self.obj = c_void_p(lib.NCCoder_new(c_int(batchsize), c_int(pktsize)))
		l = pktsize + ((batchsize * ORDER) >> 3) + 2
		self.buf = '\x00' * l
	def genPacket(self, cache, cachesize):
		lib.NCCoder_genPacket(self.obj, cast(self.buf, POINTER(c_ubyte)), cast(cache, POINTER(c_ubyte)), c_int(cachesize))
		return self.buf


class NIODecoder(object):
	@classmethod
	def tofile(cls, path, size, complete_callback, hash):
		self = cls(16, int(math.ceil(size/float(1024))), 1024, 8, complete_callback, hash[-8:])
		self.filepath = path
		self.filesize = size
		return self
	def __init__(self, batchsize, pktnum, pktsize, ff, complete_callback, tag):
		self.done = False
		self.finish = False
		self.M = batchsize
		self.K = pktnum
		self.T = pktsize
		self.F = ff
		self.buf = (c_ubyte * (self.K * self.T))(0)
		self.numDec = c_int(0)
		self.numReceived = c_int(0)
		self.complete_callback = complete_callback
		lib.NonBlockingDecoder_new.restype = c_void_p
		MYFUNC = CFUNCTYPE(c_int)
		def NIODecoderCallback():
			NIODecoder.onComplete(self)
			self.complete_callback()
			return 0

		self.callback = MYFUNC(NIODecoderCallback)
		self.obj = c_void_p(lib.NonBlockingDecoder_new(c_int(batchsize), c_int(pktnum), c_int(pktsize),
			c_int(ff), cast(self.buf, POINTER(c_ubyte)), byref(self.numDec), byref(self.numReceived), self.callback, tag))

	def receivePacket(self, recv):
		if not self.done:
			lib.bufWrite(self.obj, cast(recv, POINTER(c_ubyte)))

	def waitDecodingThread(self):
		lib.NonBlockingDecoder_wait(self.obj)

	def onComplete(self):
		self.done = True
		buf = ctypes.string_at(self.buf, self.filesize)
		open(self.filepath, 'w+').write(buf)
		lib.bufFree(cast(self.obj, POINTER(c_ubyte)))
		#print 'onComplete'
		self.finish = True

	 # do not use 'self.done' flag.
	 # Because onComplete will write file to disk, the time for it to complete is huge.
	def complete(self):
		return self.done
		
		# lib.BatsDecoder_complete.restype = c_bool
		# return lib.BatsDecoder_complete(self.obj, c_double(1.0))


	def getDecoded(self):
		return self.numDec.value
	
	def getReceived(self):
		return self.numReceived.value

	def getBufferFillRatio(self):
		lib.bufGetFilledRatio.restype = c_double
		return lib.bufGetFilledRatio(self.obj)



if __name__ == "__main__":
	import sys
	#M = BATCH_SIZE
	#K = 512
	#T = 4
	#ff = 8
	
	#test = []
	
	#for i in range(0, K):
	#	for j in range(0, T):
	#		test.append(chr((i+1)%64 + (j%4)*64))
	
	#print test
	
	#enc = Encoder(M, K, T, ff)
	#dec = Decoder(M, K, T, ff)
	
	#enc.setInputPackets(test)
	
	#for i in range(0, M):
	#	send = enc.genPacket()
	#	print send
	
	# Create encoder instance given source filename.
	enc2 = Encoder.fromfile(sys.argv[2])
	enc = Encoder.fromfile(sys.argv[1])

	import hashlib

	def cal_hash(f, block_size):
		while True:
			data = f.read(block_size)
			if not data:
				break
			hashlib.sha256().update(data)
		return hashlib.sha256().hexdigest()
	
	myfile2 = open(sys.argv[2], 'rb')
	myfile = open(sys.argv[1], 'rb')

	# Create decoder instance given destination filename and filesize.
	def a():
		print 'a()'
	dec2 = NIODecoder.tofile('output2', os.path.getsize(sys.argv[2]), a, cal_hash(myfile2, 512*1024))
	dec = NIODecoder.tofile('output', os.path.getsize(sys.argv[1]), a, cal_hash(myfile, 512*1024))
	#NIODecoder.onComplete(self)

	myfile2.close()
	myfile.close()
	
	# Create recoder instance
	print "python-create: packetSize = %d" % PACKET_SIZE
	rec2 = Recoder(BATCH_SIZE, PACKET_SIZE)
	rec = Recoder(BATCH_SIZE, PACKET_SIZE)
	
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
	#for i in range(PACKET_SIZE0):
	#	enc.genPacket()
	#print "Ended...", time.time()-t0
	#sys.exit(1)
	buf = []
	while not dec.complete() or not dec2.complete():
		send = enc2.genPacket()
		if send:
			dec2.receivePacket(send)

		send = enc.genPacket() # Generate a packet
		#Test for new batch
		if send == '':
			print "New Batch!"
			# Recode
			if len(buf) > 0:
				tmp = ''.join(buf)
				for i in range(0, BATCH_SIZE):
					if random() >= r:
						resend = rec.genPacket(tmp, len(buf))
						dec.receivePacket(resend)
						Rcnt += 1
					else:
						print "Packet lost in second link"
				# reset buffer
				del buf[0:len(buf)]
			else:
				print "All packets in batch lost"
			continue
		#simulate channel
		if random() >= r:
			buf.append(send)
		else:
			print "Packet lost in first link"
		#print send
		#print len(send)
		#dec.receivePacket(send) # Receive a packet
		#print "Dec::" + str(cnt) + "/" + str(dec.getDecoded()) + ", recv'd/dec'd, BatchID = " + str(ord(send[0])+256*ord(send[1]))
		Scnt += 1
		print 'Fill ratio', dec.getBufferFillRatio()
	print "Dec::" + str(Scnt) + "/" + str(Rcnt) + "/" + str(dec.getDecoded()) + ", sent/recv'd/dec'd"
	print "Ended...", time.time()-t0
	
	#result = [chr(x) for x in dec.buf]
	
	#print result
	
	#if test == result:
	#	print "Success!"
	#else:
	#	print "Error!"
	#	l = min(len(test), len(result))
	#	for i in range(0, l):
	#		if test[i] == result[i]:
	#			print str(i) + "-th byte same"
	#		else:
	#			print str(i) + "-th byte different: " + str(ord(test[i])) + " VS " + str(ord(result[i]))
	#	print "Length of test: " + str(len(test)) + ", Length of result: " + str(len(result))

