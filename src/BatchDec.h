#ifndef BATCHDEC_H_
#define BATCHDEC_H_

#include <cstdlib>
#include <cstdio>
#include <cstring>
//#include "Utilities.h"
//#include "FiniteField.h"
#ifdef __cplusplus
#include "BatsBasic.h"
#include "platform.h"
#include <algorithm>
#include <vector>

#include "InactDecoder.h"
#include "Partition.h"

#include "VariableNode.h"
#include "CheckNode.h"

#include "DecPacketBuilder.h"

#include <cassert>

using namespace std;


class BatsDecoder : public BatsBasic{
protected:

    // current batch ID
    KeyType batchID;

    // number of received packets
    int nloss1,nloss2,nloss3;
    int nRecPkg;
    
    int nSavedPkg;
    
    // for sliding window position and mode
    PosType fromP; // the position current window start
	PosType windowS; // the size of current window, 0 is reserved for largest window
	ModeType modeT;  // a real number from -1 to 1, meaning slope factor

public:     
    // number of total decoded packets, including check packets
    int nDecoded;
    
    // number of decoded original packets
    int nDecodedPkg;

    // number of decoded packet within current window
    int nDecodedInTime;

    // evaluation range
    long evalFrom;
    long evalTo;
    // number of decoded packet within evaluation range
    int nDecodedPkginEval;


    double complete_flag;
    
protected:
    // Sparse matrix decoding
    int nRecBatch;
    CheckNode* batchSet[BATSDECODER_MAXBATCH]; // save received batch
    
    vector<VariableNode> var;

    ArrayQueue<CheckNode*> *decQueue;

    // int inactRedun; // the redundancy in decoding inactive 
    int receiRedun; // the redundancy in received packets
    int step_for_packets;
    int accumulate_packets;
    int num_inact;

    int maxInact; //maximum number of inactivated variable nodes allowed

    //temp variables
    SymbolType** GH;
    SymbolType** invMat;
	
	//Tom: added new class, 17 July, 2013.
	InactDecoder* inactDecoder;
	//Tom: added on 29 July, 2013.
	DecPacketBuilder* packBuilder;
    
public:

BatsDecoder(int M, int K, int T, SymbolType *output, LDPCStruct* ldpcin = NULL, long evalFrom = 0, long evalTo = 0, int randseed=0):
	BatsBasic(M,K,T,ldpcin,randseed),
	evalFrom(evalFrom),
	evalTo(evalTo),
	nDecodedPkginEval(0),
	nDecodedInTime(0),
	fromP(0),
	windowS(0),
	modeT(0.0)
	{

	    complete_flag = 0.0;
        setOutputPacket(output);
        // Sparse Matrix Decoding
        nloss1 = 0;
        nloss2 = 0;
        nloss3 = 0;
        
        nRecPkg = 0;
        nSavedPkg = 0;
        nRecBatch = ldpcNum;
        nDecoded = 0;
        nDecodedPkg = 0;


        decQueue = new ArrayQueue<CheckNode*>(BATSDECODER_MAXBATCH);
             
        // maximum allowed number of inactive variables
        // maxInact = 4*sqrt(packetNum);
        maxInact = max(hdpcNum + additionalPerminactNum, 100);
		//maxInact = 12;//test
        
        d_log("BatsDecoder: maxInact = %d\n", maxInact);
        //receiRedun = 0.05*packetNum;
		receiRedun = 0.02*packetNum;
		step_for_packets = 0.001*packetNum;
		accumulate_packets = 0;
		num_inact = 0;

        // inactRedun = maxInact/10;
        // inactRedun = (inactRedun > 8)? inactRedun:8;
		
        // initial variable nodes
		VariableNode::SetBuffer(totalNum, maxInact);
        int idx = 0;
        for(int i = 0; i<totalNum; i++) {
            if (layout->inPerminactPart(i)) {
				var.push_back(VariableNode(i, buf->GetPacket(i), idx));
                idx++;
            } else {
				var.push_back(VariableNode(i, buf->GetPacket(i)));
			}
        }
        // init batchSet
        for(int i=ldpcNum;i<BATSDECODER_MAXBATCH;i++){
            batchSet[i] = NULL;
        }
                
        // init temp variables
        GH = mallocMat<SymbolType>(batchSize,batchSize);
        invMat = mallocMat<SymbolType>(batchSize,batchSize);
        
		//Init InactDecoder
		inactDecoder = new InactDecoder(*precode, hdpcNum + additionalPerminactNum, maxInact, hdpcNum, packetSize);
		//init decoder side packet builder
		packBuilder = new DecPacketBuilder(var, builder->GetRand(), batchSize);
		//Hook up
		builder->SetPacketBuilder(packBuilder);
		
        // LDPC parameters init
		int dataActive = packetNum - additionalPerminactNum;
        if (ldpcNum > 0) {    
            int ldpcBlockNum = dataActive / ldpcNum;           
            int ldpcMaxCheckDeg = ldpcVarDegree * (ldpcBlockNum + 1) + 1 + 2; // including pi part
            
            for(int i = 0; i<ldpcNum; i++){
                batchSet[i] = new CheckNode(i, 1, packetSize, maxInact);//id = i
                CheckNode* it = batchSet[i];
                it->codingVec[0][0] = 1;
                it->codingRank = 1;
                it->numRec++;
                memset(it->packet[0], 0, packetSize);
            }

            //int nb, nc, nd;
            //for (int i = 0; i < dataActive; i++) {
            //    nb = i / ldpcNum;
            //    nc = i % ldpcNum;
            //    for (int j = 0; j < ldpcVarDegree; j++) {
            //        nd = (nc + j * nb + j) % ldpcNum;
            //        BEdge* newEdge = batchSet[nd]->addEdge(&(var[i]), 1);
            //        newEdge->g[0] = 1;
            //        newEdge->gh[0] = 1;
            //    }
            //}

            // read LDPC links from the LDPCstruct class - Added by Kairan Sun
			for (int i = 0; i < dataActive; i++) {
				for(set<int>::iterator it = ldpc->native2ldpc[i].begin(); it != ldpc->native2ldpc[i].end(); ++it){
					BEdge* newEdge = batchSet[(*it)]->addEdge(&(var[i]), 1);
					newEdge->g[0] = 1;
					newEdge->gh[0] = 1;
				}
			}

            for (int i = 0; i < ldpcNum; i++) {
                BEdge* newEdge = batchSet[i]->addEdge(&(var[i+packetNum]), 1);
                newEdge->g[0] = 1;
                newEdge->gh[0] = 1;
            }
            // add inactive edges in the end
            // not applicable for sliding window Raptor scheme
            if((hdpcNum + additionalPerminactNum) > 0){
				for (int j = 0; j < ldpcNum; j++) {
					for (int i = 0; i < 2; i++) {
						int k = layout->PerminactToActual((j + i) % (hdpcNum + additionalPerminactNum));
						// Added by Kairan Sun for sliding window Raptor
						if(!(ldpc->addNewEdgeBetweenNativeAndLdpc(k,j))) continue;
						// end of addition
						BEdge* newEdge = batchSet[j]->addEdge(&(var[k]), 1);
						newEdge->g[0] = 1;
						newEdge->gh[0] = 1;
						batchSet[j]->inactCoef[0][var[k].inactSeq] = 1;
					}
				}
            }
        }
        // Added by Kairan - add the warm-up/cool-down packets in the front/back
        // if(evalFrom<0) evalFrom = 0;
		// if(evalFrom>=packetNum) evalFrom = packetNum-1;
		// if(evalTo<0) evalTo = 0;
		// if(evalTo>=packetNum) evalTo = packetNum-1;
        if( evalFrom < evalTo) {
        	// warm-up
        	int batchCnt = ldpcNum-1;
        	decQueue->empty();
        	CheckNode* it;
			for (int i = 0; i < evalFrom; i++) {
				batchCnt++;
				batchSet[batchCnt] = new CheckNode(batchCnt, 1, packetSize, maxInact);
				it = batchSet[batchCnt];
				it->codingVec[0][0] = 1;
				it->codingRank = 1;
				it->numRec++;
				memset(it->packet[0], 'A', packetSize); // padding with 'A'
				BEdge* newEdge = it->addEdge(&(var[i]), 1);
				newEdge->g[0] = 1;
				newEdge->gh[0] = 1;
				tryPushDecQueue(it);
			}
			// cool-down
			for (int i = evalTo+1; i < packetNum; i++) {
				batchCnt++;
				batchSet[batchCnt] = new CheckNode(batchCnt, 1, packetSize, maxInact);
				it = batchSet[batchCnt];
				it->codingVec[0][0] = 1;
				it->codingRank = 1;
				it->numRec++;
				memset(it->packet[0], 'A', packetSize); // padding with 'A'
				BEdge* newEdge = it->addEdge(&(var[i]), 1);
				newEdge->g[0] = 1;
				newEdge->gh[0] = 1;
				tryPushDecQueue(it);
			}
			// decode all warm-up/cool-down vars
			while(decQueue->isNonEmpty()){
				decodeBatch();
			}
        }
        else{
        	this->evalFrom = 0; // means all packets are evaluated
			this->evalTo = totalNum - 1;
		}
    }

    ~BatsDecoder(){
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

    void setOutputPacket(SymbolType *output){
		buf->SetBuffer(output);
    }

    inline int complete(double decRatio){
	    //return (nDecodedPkg>=packetNum * decRatio);
    	double ratio = ((double) nDecodedPkg) / ((double)packetNum);
    	if(ratio > complete_flag) {
    		complete_flag = ratio;
    	}
    	return (complete_flag >= decRatio);
    }

    inline double completeInEval(){
    	return ((double)nDecodedPkginEval / (double)(evalTo - evalFrom + 1));
    }

    inline double completeInTime(){
    	return ((double)nDecodedInTime / (double)(evalTo - evalFrom + 1));
	}

    void rankDist(double* rd);
    void logRankDist();
	//Number of times inactivation decoding is triggered
    int numInact() {
	    return num_inact;
    }
    void receivePacket(SymbolType *packet);


    bool inactDecoding(){
		bool result;
		d_log("inactDecoding: nRecPkg = %d, loss(decoded) = %d, loss(dependant) = %d\n", nRecPkg, nloss1, nloss2);

		result = inactDec();

		//if inactDecoding fail, we consider it needs more packets and retry inactDecoding again

		if(result) {
			complete_flag = 1.0;
			//nDecodedPkg = packetNum; // comment out to only count BP decoding results as decoded pkg number
			//nDecodedPkginEval = evalTo - evalFrom + 1;
		}
		else {
			complete_flag = ((double) nDecodedPkg) / ((double)packetNum);
		}
		/* prepare for the next run */
		accumulate_packets = 0;
		return result;
    }





    bool readyForInact() {
		/*
        	bool re = (nSavedPkg >= packetNum + receiRedun);
        	if (re)
			d_log("Inactivation Decoding ready");
		*/

	    if ((nSavedPkg >= packetNum + receiRedun) &&
		(accumulate_packets >= step_for_packets)) {
		    //cout << "accumulate_packets = " << accumulate_packets << endl;
            num_inact++;
		    return true;
	    }
	    else {
		    return false;
	    }
    }

    int getDecodedPkg(){
        int n = 0;
        for(int j = 0; j < packetNum; j++){
            if(var[j].decoded){
                n++;
            }
        }
        return n;
    }
protected:
    CheckNode* initNewBatch(KeyType);
    void tryPushDecQueue(CheckNode*);
    void decodeBatch();
    bool addInact();
    bool inactDec();
};

#else
typedef struct BatsDecoder BatsDecoder;
#endif

#ifdef __cplusplus
extern "C" {
#endif
    
    BatsDecoder* BatsDecoder_new(int M, int K, int T, SymbolType *output);
    int BatsDecoder_complete(BatsDecoder* decoder, double decRatio);
    int BatsDecoder_getDecoded(BatsDecoder* decoder);
    void BatsDecoder_rankDist(BatsDecoder* decoder, double* rd);
    void BatsDecoder_receivePacket(BatsDecoder* decoder, uint8_t *batch);
	int BatsDecoder_readyForInact(BatsDecoder* decoder);
	int BatsDecoder_inactDecoding(BatsDecoder* decoder);

    // Patch to expose interface for setting the degree distribution. Tom Lam on 11 July, 2012.
    void BatsDecoder_setDegreeDist(BatsDecoder* decoder, double* degreeDist, int maxDeg);
    void BatsDecoder_selectDegree(BatsDecoder* decoder);

	void BatsDecoder_logRankDist(BatsDecoder *decoder);
	int BatsDecoder_numInact(BatsDecoder *decoder);
    
#ifdef __cplusplus
}
#endif


#endif /* BATCHDEC_H_ */

