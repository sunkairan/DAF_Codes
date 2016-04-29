#ifndef DELAYDEC_H_
#define DELAYDEC_H_

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
//#include "Utilities.h"
//#include "FiniteField.h"

#include "BatchDec.h"
#include "platform.h"
#include "DecPacketBuilder.h"

using namespace std;


class DelayDecoder : public BatsDecoder{
private:
    //PosType fromP;
    //PosType windowS;
    //ModeType modeT;
    //long evalFrom;
    //long evalTo;
    
public:
	DelayDecoder(int K, int T, SymbolType *output, LDPCStruct* ldpcin);
	DelayDecoder(int K, int T, SymbolType *output, LDPCStruct* ldpcin, long evalFrom, long evalTo, int randseed=0);

    ~DelayDecoder(){
        for(int i=0;i<BATSDECODER_MAXBATCH;i++){
            if(batchSet[i] != NULL)
                delete batchSet[i];
        }

        freeMat(GH,batchSize);
        freeMat(invMat,batchSize);
		
		VariableNode::FreeBuffer();

        delete decQueue;
		delete packBuilder;
    }

    CheckNode* initNewBatch(KeyType batchID);
    void receivePacket(SymbolType *rawPacket);
    void receivePacket(SymbolType *rawPacket, bool canFinish);
};





#endif /* DELAYDEC_H_ */

