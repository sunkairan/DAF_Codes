#import gevent.socket
import hashlib
import httplib, re

def hashFile(path):
	f = open(path)
	m = hashlib.sha256()
	while True:
		s = f.read(4096)
		if len(s) == 0:
			break
		m.update(s)
	return m.hexdigest()


#	v = ((5*(20-int(math.log(connected+1,2))))*math.log((5*(20-int(math.log(connected+1,2)))),10)+int(math.log(numReceived/BATCH_SIZE+0.1,2)**2))*math.log10(bandwidth*10+2)
#	return int(v)

if __name__ == "__main__":
	s = "hello\x00"
	print "hash: %s"%(hashFunc(s))
