#include <cstdlib>
#include <cstring>
#include "Utilities.h"
#include "FiniteField.h"


#ifndef NCCODER_H
#define	NCCODER_H

#ifdef __cplusplus

class NCCoder {
public:
    int L; //total packet length in Bytes
    int Q; //No. of element in finite field minus one. For use in PRNG

    //Access to PRNG 
    MTRand *psrand;

public:

    NCCoder(int batchSize, int packetSize) {
        psrand = new MTRand(5666);
        Q = FF.size - 1;
        //Calculate total packet length (with header and key)
        L = (batchSize * (FF.order) / SYMBOLSIZE) + packetSize + 2;
	//cout << "packetSize = " << packetSize << "  L = " << L << endl;
    }

    ~NCCoder() {
        delete psrand;
    }

    void genPacket(SymbolType *dstPacket, SymbolType *cache, int cacheSize); //Rmk: any better name than cacheSize?
};

#else
typedef
struct NCCoder
    NCCoder;
#endif


#ifdef __cplusplus
extern "C" {
#endif

    NCCoder* NCCoder_new(int batchSize, int packetSize);
    void NCCoder_genPacket(NCCoder* recoder, SymbolType *dstPacket, SymbolType *cache, int cacheSize);

#ifdef __cplusplus
}
#endif


#endif	/* NCCODER_H */

