/* 
 * File:   BatsBasic.h
 * Author: Shenghao Yang
 *
 * Created on July 17, 2011, 10:39 AM
 */

#ifndef BATSBASIC_H
#define	BATSBASIC_H
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include "Utilities.h"
#include "FiniteField.h"
#include "vand.h"

#include "RawPacket.h"
#include "PacketSampler.h"
#include "Precode.h"
#include "PacketBuffer.h"

#include "BatchBuilder.h"

#include <algorithm>

#define BATSDECODER_MAXBATCH 65535

#include <platform.h>

class BatsBasic {
protected:
    double precodeRate;
    // Basic encoding parameters
    int batchSize; // batch size
    int packetNum; // packet number, not including check packets//
    int packetSize; // packet length in Bytes
    int checkNum; // number of check packets
    int totalNum; // packetNum + checkNum

    // precode parameters
    int ldpcNum; // number of LDPC packets
    int hdpcNum; // number of HDPC packets
	int additionalPerminactNum;
    
    int ldpcVarDegree; // variable degree of LDPC
    int piDegree; // PI degree in a batch

    // finite field and parameters
    int fieldSizeMinOne;
    int fieldOrder;

#ifdef USE_VANDERMOND
    struct vandermond_matrix *vand_matrix;
#endif
	
	//Tom: added 4 July, 2013.
	RawPacket *curPacket;
	//Tom: added 16 July, 2013.
	PrecodeLayout *layout;
	//Tom: added 16 July, 2013.
	Precode *precode;
	//Tom: added 19 July, 2013.
	PacketBuffer *buf;
	//Tom: added 29 July, 2013.
	BatchBuilder *builder;

	//int maxInact; // the maximum allowed number of inactivation 
	//int maxRedun; // the maximum redundancy used to decode inactive packets
public:
    BatsBasic(int M, int K, int T, bool noPrecode=false, int randseed = 0) : batchSize(M), packetNum(K), packetSize(T) {
        precodeRate = 0.01;
        
        if(noPrecode) {
        	//ldpcNum = (int) (precodeRate * packetNum + sqrt(2*packetNum));
        	ldpcNum = 0;//3;
			hdpcNum = 2;
			ldpcVarDegree = 0;//3;
			checkNum = ldpcNum+hdpcNum;
			totalNum = packetNum + checkNum;
			additionalPerminactNum = 0;
			piDegree = 0;
			piDegree = min(piDegree, hdpcNum + additionalPerminactNum);
        }
        else {
            ldpcNum = (int) (precodeRate * packetNum + sqrt(2*packetNum));

            if (ldpcNum > 0) {
                hdpcNum = 5+(int)log(packetNum);
                //hdpcNum = 0; // for testing only

            } else { // BATS_PRECODE_RATE == 0 for no precoding
                hdpcNum = 0;
            }
            ldpcVarDegree = 3;

            checkNum = ldpcNum+hdpcNum;
            
            totalNum = packetNum + checkNum;
            
            additionalPerminactNum = log(packetNum);
            // piDegree = sqrt(batchSize)+2;
            piDegree = 2;
            piDegree = min(piDegree, hdpcNum + additionalPerminactNum);
        }
            

		//d_log("totalNum = %d, ldpcNum = %d, hdpcNum = %d, smNum = %d, piNum = %d, smMinLd = %d, piMinHd = %d, piDegree = %d\n",
	    //  totalNum, ldpcNum, hdpcNum, smNum, piNum, smMinLd, piMinHd, piDegree);

        // set finite field paramters
        fieldSizeMinOne = FF.size - 1;
        fieldOrder = FF.order;
        
		//finite field param related with coding vector
        int nSymInHead = batchSize * fieldOrder / SYMBOLSIZE;
        int nFFInSym = SYMBOLSIZE / fieldOrder;
        SymbolType maskDec = 0xFF;          
        maskDec <<= (8-fieldOrder);
        SymbolType maskEnc = 0x80;
        maskEnc >>= (fieldOrder - 1);

#ifdef USE_VANDERMOND
        vand_matrix = get_vandermond_matrix(batchSize);
#endif

		buf = new PacketBuffer(packetNum, checkNum, packetSize);

		curPacket = new RawPacket(nSymInHead, nFFInSym, fieldOrder, maskEnc, maskDec, batchSize);
		layout = new PrecodeLayout(packetNum, ldpcNum, hdpcNum, additionalPerminactNum);
		precode = new Precode(*layout, ldpcVarDegree);
		
		builder = new BatchBuilder(*layout, packetNum + ldpcNum - additionalPerminactNum, piDegree, batchSize,randseed);
	}

    ~BatsBasic(){
		delete curPacket;
		delete precode;
		delete layout;
		delete buf;
		
		delete builder;
    }

    void selectDegree() {
		stringstream iss;
		iss << "ddM" << batchSize << "m" << fieldOrder << "r92TO.txt";
		builder->SetDegreeDist(iss.str());
    }

    void setDegreeDist(double* degreeDist, int maxDeg) {
		builder->SetDegreeDist(degreeDist, maxDeg);
    }

    inline int getSmallestBid(){
        return ldpcNum;
    }
    
private:
};

#endif	/* BATSBASIC_H */

