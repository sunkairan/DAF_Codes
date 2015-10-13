ODIR = build
RDIR = release
SDIR = src

SHARED = shared
STATIC = static


CXX = g++
CC = gcc
CCFLAGS = -g
CXXFLAGS = -g -I$(SDIR) -I$(SDIR)/util -mtune=native #-DSSE_DEBUG -DPRINT_DEBUG -DLOG_DEBUG

_ALLUTIL = FiniteField.h Utilities.h MersenneTwister.h CircularBuf.h
ALLUTIL = $(_ALLUTIL:%=$(SDIR)/util/%)

_OBJ = util/FiniteField.o BatchEnc.o BatchDec.o NCCoder.o NIODecoder.o util/ff.o\
util/CircularBuf.o init.o vand.o Precode.o InactDecoder.o VariableNode.o\
DelayEnc.o DelayDec.o



STATICOBJ = $(_OBJ:%=$(ODIR)/$(STATIC)/%)
SHAREDOBJ = $(_OBJ:%=$(ODIR)/$(SHARED)/%)

DIRS_DEP_STATIC = $(ODIR) $(ODIR)/$(STATIC) $(ODIR)/$(STATIC)/util
DIRS_DEP_SHARED = $(ODIR) $(ODIR)/$(SHARED) $(ODIR)/$(SHARED)/util

all: staticlib sharedlib testapp udpsender

$(ODIR)/$(STATIC)/util/ff.o: 
		$(CXX) $(CXXFLAGS) -c src/util/ff.c -o $@

$(ODIR)/$(SHARED)/util/ff.o:
		$(CXX) $(CXXFLAGS) -fPIC -c src/util/ff.c -o $@

$(ODIR)/$(STATIC)/init.o: 
		$(CXX) $(CXXFLAGS) -c src/init.c -o $@

$(ODIR)/$(SHARED)/init.o:
		$(CXX) $(CXXFLAGS) -fPIC -c src/init.c -o $@

$(ODIR)/$(STATIC)/%.o : $(SDIR)/%.cpp $(SDIR)/%.h | $(DIRS_DEP_STATIC)
		$(CXX) -c -o $@ $< $(CXXFLAGS)

$(ODIR)/$(SHARED)/%.o : $(SDIR)/%.cpp $(SDIR)/%.h | $(DIRS_DEP_SHARED)
		$(CXX) -fPIC -c -o $@ $< $(CXXFLAGS)

$(ODIR):
		mkdir $(ODIR)
$(ODIR)/$(STATIC): | $(ODIR)
		mkdir $(ODIR)/$(STATIC)
$(ODIR)/$(SHARED): | $(ODIR)
		mkdir $(ODIR)/$(SHARED)
$(ODIR)/$(STATIC)/util: | $(ODIR)/$(STATIC)
		mkdir $(ODIR)/$(STATIC)/util
$(ODIR)/$(SHARED)/util: | $(ODIR)/$(SHARED)
		mkdir $(ODIR)/$(SHARED)/util

$(ODIR)/$(STATIC)/util/FiniteField.o : $(SDIR)/util/Utilities.h $(SDIR)/util/MersenneTwister.h
$(ODIR)/$(STATIC)/BatchEnc.o $(ODIR)/$(STATIC)/BatchDec.o : $(SDIR)/BatsBasic.h

$(ODIR)/$(SHARED)/util/FiniteField.o : $(SDIR)/util/Utilities.h $(SDIR)/util/MersenneTwister.h
$(ODIR)/$(SHARED)/BatchEnc.o $(ODIR)/$(SHARED)/BatchDec.o : $(SDIR)/BatsBasic.h

$(ODIR)/$(STATIC)/DelayEnc.o : $(SDIR)/BatchEnc.cpp $(SDIR)/BatchEnc.h $(SDIR)/BatsBasic.h
$(ODIR)/$(STATIC)/DelayDec.o : $(SDIR)/BatchDec.cpp $(SDIR)/BatchDec.h $(SDIR)/BatsBasic.h
$(ODIR)/$(SHARED)/DelayEnc.o : $(SDIR)/BatchEnc.cpp $(SDIR)/BatchEnc.h $(SDIR)/BatsBasic.h
$(ODIR)/$(SHARED)/DelayDec.o : $(SDIR)/BatchDec.cpp $(SDIR)/BatchDec.h $(SDIR)/BatsBasic.h


$(STATICOBJ) : $(ALLUTIL)
$(SHAREDOBJ) : $(ALLUTIL)

staticlib: $(STATICOBJ)
		ar -cvr $(ODIR)/libbatscore.a $(STATICOBJ)
		cp $(ODIR)/libbatscore.a $(RDIR)/
		cp $(ODIR)/libbatscore.a test/
		cp $(ODIR)/libbatscore.a udp/


sharedlib: $(SHAREDOBJ)
		$(CXX) -shared -o $(ODIR)/libbatscore.so $(SHAREDOBJ)
		cp $(ODIR)/libbatscore.so $(RDIR)/
		cp $(ODIR)/libbatscore.so test/
		cp $(ODIR)/libbatscore.so udp/
		cp $(RDIR)/* test/
		cp $(RDIR)/* udp/
		cp $(RDIR)/* schfile/

testapp:
		$(MAKE) -C test

udpsender:
		$(MAKE) -C udp

.PHONY: clean
clean:
		rm -rf $(ODIR)
		#rm test/test_ff
		#rm test/test_ff.o
		#rm test/bats.py
		#rm test/libbatscore.so
		#rm test/rankDist.txt
		#rm test/ddM16m8r92TO.txt
		#rm test/output


# List of header file dependencies
#$(SDIR)/BatchDec.h : $(SDIR)/BatsBasic.h
#$(SDIR)/BatchEnc.h : $(SDIR)/BatsBasic.h
#$(SDIR)/BatsBasic.h : $(SDIR)/util/Utilities.h $(SDIR)/util/FiniteField.h
#$(SDIR)/NCCoder.h : $(SDIR)/util/Utilities.h $(SDIR)/util/FiniteField.h
#$(SDIR)/util/FiniteField.h : $(SDIR)/util/Utilities.h
#$(SDIR)/util/Utilities.h : $(SDIR)/util/MersenneTwister.h

