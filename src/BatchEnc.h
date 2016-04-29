#ifndef BATCHENC_H_
#define BATCHENC_H_

#include <cstring>
#include <fstream>
#include <iostream>
//#include "Utilities.h"
//#include "FiniteField.h"

#include "EncPacketBuilder.h"

#ifdef __cplusplus
#include "BatsBasic.h"
#include "platform.h"
using namespace std;


class BatsEncoder : public BatsBasic{
protected:

    // current batch ID
    KeyType batchID;
    int genPktCounter;


    SymbolType **batch;
    SymbolType **batchWithoutId;
	
	//Keep a table of pointers to right location in the batch
	SymbolType** payload;

    /* tem, maybe need to remove it*/
    KeyType id;
	
	//Tom: added in 29 July, 2013.
	EncPacketBuilder* packBuilder;

	// for sliding window position and mode
	PosType fromP; // the position current window start
	PosType windowS; // the size of current window, 0 is reserved for largest window
	ModeType modeT;  // a real number from -1 to 1, meaning slope factor
public:
	// evaluation range
	long evalFrom;
	long evalTo;
public:
  
    BatsEncoder(int M, int K, int T, SymbolType *input, LDPCStruct* ldpcin = NULL, long evalFrom = 0, long evalTo = 0, int randseed=0) :
    	BatsBasic(M,K,T,ldpcin,randseed),
		evalFrom(evalFrom),
		evalTo(evalTo),
		fromP(0),
		windowS(0),
		modeT(0){
        // init batch ID
        batchID = getSmallestBid();
        // Added by Kairan - add warm-up/cool-down periods
        // if(evalFrom<0) evalFrom = 0;
		// if(evalFrom>=packetNum) evalFrom = packetNum-1;
		// if(evalTo  <0) evalTo = 0;
		// if(evalTo  >=packetNum) evalTo   = packetNum-1;
        if(evalTo > evalFrom) {
        	batchID += evalFrom; // warm-up
        	batchID += (packetNum-evalTo-1); //cool-down
        	memset(input, 'A',evalFrom*packetSize); // padding with 'A'
        	memset(input+evalTo*packetSize+1, 'A',(packetNum-evalTo)*packetSize-1);
        }
        else{
        	evalFrom = 0; // means all packets are evaluated
        	evalTo = totalNum - 1;
        }
		
		
		genPktCounter = batchSize;
		int packetSizeWithHeaderAndId = packetSize + batchSize*fieldOrder/8 + sizeof(KeyType);
		
		/* Encoder hold the batch memorty */
		batch = mallocMat<SymbolType>(batchSize, packetSizeWithHeaderAndId);
		batchWithoutId = (SymbolType **)malloc(batchSize * sizeof(SymbolType *));
		for (int i = 0; i < batchSize; i++) {
			batchWithoutId[i] = batch[i] + sizeof(KeyType);
		}
		
        setInputPackets(input);
		
		payload = (SymbolType**)malloc(batchSize*sizeof(SymbolType*));
		
		packBuilder = new EncPacketBuilder(*buf, payload, builder->GetRand(), packetSize);
		//Hook up
		builder->SetPacketBuilder(packBuilder);
	}
	~BatsEncoder() {
		free(payload);
		delete packBuilder;
	}

    /* Inernal function */
    void setInputPackets(SymbolType *input);

    // generate a batch, return the degree
    // batch is a batch without key
    void genBatchWithKey(SymbolType **batch, KeyType key);

    SymbolType *genPacket() {

	    if (genPktCounter == batchSize) {
		    genPktCounter = 0;
		    id = genBatch(batchWithoutId);
	    }

	    saveIDInPacket(batch[genPktCounter], &id);
	    //cout << "encoder: batch_id = " << id << endl;
	    return batch[genPktCounter++];

    }
    /* Internal function;
       batch is a pointer without batch's key
    */
    KeyType genBatch(SymbolType **batch){
        KeyType key = batchID;
        batchID++;
        genBatchWithKey(batch, key);
        return key;
    }
protected:
    
    void genCheckPkg();
    
//public:
	// debug
	// verify parity check
    /*bool verifyCheckPkg(){
		int packetAndLDNum = packetNum + ldpcNum;
        SymbolType ** check = mallocMat<SymbolType>(checkNum,packetSize);
        
        // compute B_A * C_A
        for (int k = 0; k < smMinLd; k++) {
            for (int d = 0; d < ldpcVarDegree; d++) {
                FF.addvv(check[(k % ldpcNum + d * (int)(k / ldpcNum) + d) % ldpcNum], packets[k], packetSize);
            }
        }
        // compute [B_I H] * C_P
        for (int j = 0; j < ldpcNum; j++){
            for (int i = 0; i < 2; i++){
                int k = (j+i) % piNum;
                FF.addvv(check[j],getPkgHead(layout->PerminactToActual(k)),packetSize);
            }
        }
        
        // compute HDPC
        SymbolType** Bext = (SymbolType**)malloc(packetAndLDNum*sizeof(SymbolType*));    
        for (int i = 0; i < packetAndLDNum; i++){
            Bext[layout->ActualToInterlaced(i)] = getPkgHead(i);
        }
        
        MultiplyQ(&check[ldpcNum], Bext, packetSize, true);
        
        int n = 0;
        
        for (int i = 0; i < checkNum; i++){
            for (int j = 0; j < packetSize; j++){
                if (check[i][j] != checkPackets[i][j]){
                    n++;
                    cout << "Wrong check pkg: " << i << endl;
                    continue;
                }
            }
        }
        
        return (n==0);
    }*/
};

#else
typedef struct BatsEncoder BatsEncoder;
#endif


#ifdef __cplusplus
extern "C" {
#endif
    
    BatsEncoder* BatsEncoder_new(int M, int K, int T, SymbolType *input);
	//uint16_t BatsEncoder_genBatch(BatsEncoder* encoder, SymbolType **batch);
    SymbolType* BatsEncoder_genPacket(BatsEncoder* encoder);
	// Patch to expose interface for setting the degree distribution. Tom Lam on 11 July, 2012.
	void BatsEncoder_setDegreeDist(BatsEncoder* encoder, double* degreeDist, int maxDeg);
	void BatsEncoder_selectDegree(BatsEncoder* encoder);
	int BatsEncoder_getSmallestBid(BatsEncoder* encoder);
    
#ifdef __cplusplus
}
#endif


#endif /* BATCHENC_H_ */
