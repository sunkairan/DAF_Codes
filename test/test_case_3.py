# The following modules are required: ctypes, os, math, random
from ctypes import *
import ctypes

import os, sys
import math
from random import *
import hashlib
from bats import Encoder, Decoder, NIODecoder, Recoder
import subprocess

PACKET_SIZE = 1024


g_decoding_finised = False

def callback():
    global g_decoding_finised
    g_decoding_finised = True


def cal_hash(f, block_size):
    while True:
        data = f.read(block_size)
        if not data:
            break
        hashlib.sha256().update(data)
        return hashlib.sha256().hexdigest()

myfile = open(sys.argv[1], 'rb')

dec = NIODecoder.tofile('output', os.path.getsize(sys.argv[1]), callback, cal_hash(myfile, 512*1024))

myfile.close()

def main():
    global g_decoding_finised

    enc = Encoder.fromfile(sys.argv[1])    

    # Create recoder instance
    rec = Recoder(16, PACKET_SIZE)

    # channel loss rate
    r = 0.05
    buf = []

    while not g_decoding_finised:

        send = enc.genPacket() # Generate a packet

    # new batch
        if send == '':

            if len(buf) > 0:
                tmp = ''.join(buf)
                for i in xrange(0, 16):
                    if random() >= r:
                        resend = rec.genPacket(tmp, len(buf))
                        dec.receivePacket(resend)

                del buf[0:len(buf)]
        # old batch
        else:
	#simulate channel
            if random() >= r:
                buf.append(send)

    dec.waitDecodingThread()
    ret = subprocess.call(['diff',
                           sys.argv[1],
                           'output'])
    sys.exit(ret)
    

if __name__ == "__main__":
    main()

