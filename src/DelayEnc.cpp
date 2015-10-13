#include "DelayEnc.h"

#include <algorithm>

DelayEncoder::DelayEncoder(int K, int T, SymbolType *input,long evalFrom, long evalTo, int randseed) :
        BatsEncoder(1, K, T, input, true,evalFrom,evalTo,randseed),
		sentNPacket(0)
{
}

void DelayEncoder::genBatchWithKey(SymbolType **batch, KeyType key){
    int i;

    int Tout = packetSize + (batchSize * fieldOrder / SYMBOLSIZE);//need to know header len
	
    for (i = 0; i < batchSize; i++) {
		//reset packet buffer
        memset(batch[i], 0 , Tout);
		//set header for coding vector
		curPacket->SetForWriteWOKey(batch[i]);
		curPacket->SetHeader(i);
		//save location
		payload[i] = curPacket->GetPayload();
	}
	builder->Build(key, fromP, windowS, modeT);
}

SymbolType *DelayEncoder::genPacket() {
		sentNPacket ++;
		fromP   = 0;
		windowS = 0;
		modeT   = 0;
	    if (genPktCounter == batchSize) {
		    genPktCounter = 0;
		    id = genBatch(batchWithoutId);
	    }

	    saveIDInPacket(batch[genPktCounter], &id);
	    //cout << "encoder: batch_id = " << id << endl;
	    SymbolType *delayPacket = saveDelayInfoInPacket(batch[genPktCounter++]);
	    return delayPacket;

}

SymbolType *DelayEncoder::genPacket(PosType FromP, PosType WindowS, ModeType ModeT) {
		sentNPacket ++;
		fromP   = FromP;
		windowS = WindowS;
		modeT   = ModeT;
	    if (genPktCounter == batchSize) {
		    genPktCounter = 0;
		    id = genBatch(batchWithoutId);
	    }

	    saveIDInPacket(batch[genPktCounter], &id);
	    //cout << "encoder: batch_id = " << id << endl;
	    SymbolType *delayPacket = saveDelayInfoInPacket(batch[genPktCounter++]);
	    return delayPacket;

}

KeyType DelayEncoder::genBatch(SymbolType **batch){
	KeyType key = batchID;
	batchID++;
	genBatchWithKey(batch, key);
	return key;
}

SymbolType *DelayEncoder::saveDelayInfoInPacket(SymbolType *orgPacket){
    int len = packetSize + batchSize*fieldOrder/SYMBOLSIZE + sizeof(KeyType);
    SymbolType *delayPacket = new SymbolType[len + 2*sizeof(PosType) + sizeof(ModeType)];
    PosType pos = (PosType) fromP;
    memcpy(delayPacket, (SymbolType*) (&pos), sizeof(PosType));
    memcpy(delayPacket + sizeof(PosType), (SymbolType*) (&windowS), sizeof(PosType));
    memcpy(delayPacket + 2*sizeof(PosType), (SymbolType*) (&modeT), sizeof(ModeType));
    memcpy(delayPacket + 2*sizeof(PosType) + sizeof(ModeType), orgPacket, len);
    return delayPacket;
}
