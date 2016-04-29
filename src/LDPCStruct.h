/*
 * LDPCStruct.h
 *
 *  Created on: Nov 10, 2015
 *      Author: Kairan Sun
 */

#ifndef LDPCSTRUCT_H_
#define LDPCSTRUCT_H_

#include <vector>
#include <set>
using namespace std;

class LDPCStruct {
private:
	int totalNativeNum;
	int totalActiveNum;
	int totalLdpcNum;
	int ldpcSpan;
	int ldpcGroupSize;
	double nativeInterv;
	int ldpcVarDegree;
	int permInactNum;
	int hdpcNum;
	vector<int> numLdpcBefore;
	vector<int> seqOfActualPos;
	vector<int> ldpcActualPos;
	vector<int> ldpcLogicalPosAfter;
public:
	vector< set<int> > native2ldpc;
	vector< set<int> > ldpc2native;
public:
	LDPCStruct(int nPacketNum,  double nInterval, int gSize, int span, int hdNum, int piNum);
	int getAcutalSeqWithLdpc(int from, int window, int** seq);
	bool addNewEdgeBetweenNativeAndLdpc(int n, int l);
	int getLdpcNum(){ return totalLdpcNum;}
	int getHdpcNum(){ return hdpcNum;}
	int getPermInactNum(){ return permInactNum;}
	int getLdpcVarDegree(){ return ldpcVarDegree;}
	void unittest(int start, int wind);
};



#endif /* LDPCSTRUCT_H_ */
