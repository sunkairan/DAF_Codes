#ifndef DELAYENC_H_
#define DELAYENC_H_

#include <cstring>
#include <fstream>
#include <iostream>
//#include "Utilities.h"
//#include "FiniteField.h"

#include "BatchEnc.h"
#include "platform.h"
#include "EncPacketBuilder.h"

using namespace std;


class DelayEncoder : public BatsEncoder{
private:
    //double codeRate; // 0.0 < c <= 1.0
    //PosType windowSize;
    long sentNPacket;
    //ModeType mode;
public:
  
    DelayEncoder(int K, int T, SymbolType *input,long evalFrom, long evalTo,int randseed=0);
	~DelayEncoder() {
		free(payload);
		delete packBuilder;
	}

    // generate a batch, return the degree
    // batch is a batch without key
    void genBatchWithKey(SymbolType **batch, KeyType key);

    SymbolType *genPacket();
    KeyType genBatch(SymbolType **batch);
    SymbolType *genPacket(PosType FromP, PosType WindowS, ModeType ModeT);

    SymbolType *saveDelayInfoInPacket(SymbolType *delayPacket);

};


#endif /* DELAYENC_H_ */
