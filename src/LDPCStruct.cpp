/*
 * LDPCStruct.cpp
 *
 *  Created on: Nov 13, 2015
 *      Author: Kairan Sun
 */
#include <vector>
#include <algorithm>
#include <set>
#include <iomanip>
#include <iostream>
#include <math.h>

#include "LDPCStruct.h"

LDPCStruct::LDPCStruct(int nPacketNum,  double nInterval, int gSize, int span, int hdNum, int piNum):
	totalNativeNum(nPacketNum),
	totalActiveNum(nPacketNum-piNum),
	totalLdpcNum(((int)ceil((double)totalActiveNum / nInterval))*gSize),
	ldpcSpan(span),
	ldpcGroupSize(gSize),
	nativeInterv(min(nInterval, (double)totalActiveNum)),
	ldpcVarDegree(3),
	permInactNum(piNum),
	hdpcNum(hdNum),
	native2ldpc(totalActiveNum,set<int>()),
	ldpc2native(totalLdpcNum, set<int>()),
	ldpcActualPos(totalLdpcNum, 0),
	ldpcLogicalPosAfter(totalLdpcNum, 0),
	numLdpcBefore(totalActiveNum + totalLdpcNum,0),
	seqOfActualPos()
{
	double restNativeP = 0.0;
	int nativeWindow;
	int ldpcFrom, ldpcTo, ldpcNum;
	int nb, nc, nd;
	int nativeWindowTo;
	int nativei, ldpci;
	for(nativei = 0, ldpci = 0; nativei < totalActiveNum /*&& ldpci < totalLdpcNum*/; ldpci+=ldpcGroupSize){
		// mapping native packets in [nativei, nativeWindowTo) to the LDPC packets in [ldpci, ldpcTo)
		restNativeP += nativeInterv;
		nativeWindow = floor(restNativeP);
		ldpcTo = min(ldpci + ldpcSpan, totalLdpcNum);
		nativeWindowTo = min(nativei + nativeWindow, totalActiveNum);
		for(int j = nativei; j < nativeWindowTo; j++) {
			seqOfActualPos.push_back(j);
			numLdpcBefore[j] = ldpci;
		}
		numLdpcBefore[nativeWindowTo-1] = ldpci + ldpcGroupSize;
		for(int j = 0; j < ldpcGroupSize; j++){
			ldpcActualPos[ldpci+j] = totalNativeNum + ldpci + j;
			ldpcLogicalPosAfter[ldpci+j] = nativeWindowTo-1;
			seqOfActualPos.push_back(totalNativeNum + ldpci + j);
		}
		ldpcNum = ldpcTo - ldpci;
		if(ldpcNum != 0){
			for(int i = 0; i < nativeWindowTo - nativei; i++, restNativeP -= 1.0){
				nb = i / ldpcNum;
				nc = i % ldpcNum;
				for (int j = 0; j < min(ldpcVarDegree , ldpcNum); j++) {
					nd = (nc + j * nb + j) % ldpcNum + ldpci;
					//addNewEdgeBetweenNativeAndLdpc(i+nativei,nd);
					addNewEdgeBetweenNativeAndLdpc(nativeWindowTo-i-1,nd); // reverse the link to get better in-time performance
				}
			}
		}
		nativei = nativeWindowTo;
	}
	for(nativei = totalNativeNum; nativei < totalNativeNum + totalLdpcNum; nativei++){ // sequence numbers in LDPC part
		//numLdpcBefore[nativei] = totalLdpcNum;
		seqOfActualPos.push_back(nativei); // there are (totalActiveNum + 2*totalLdpcNum) elements in seqOfActualPos
	}
}
int LDPCStruct::getAcutalSeqWithLdpc(int from, int window, int** seq){
	// assert: from + window < totalActiveNum + totalLdpcNum
	int actualFrom = 0;
	int actualTo = 0; // return the sequence in [actualFrom, actualTo)
	if(from >= totalActiveNum){ // in pure LDPC area
		actualFrom = totalLdpcNum + from;
		actualTo = min(totalLdpcNum + from + window, totalActiveNum + 2*totalLdpcNum);
	}
	else { // begin in native area
		if(from == 0){
			actualFrom = 0;
		}
		else{
			actualFrom = numLdpcBefore.at(from-1) + from;
			seqOfActualPos.at(actualFrom); // for possible out-of-range exception
		}
		if(from+window == 0){
			actualTo = 0;
		}
		else if(from+window >= totalActiveNum){ // for partial LDPC range, gives all the LDPC packets left
			actualTo = totalActiveNum + totalLdpcNum;
		}
		else { // for pure native range
			actualTo = numLdpcBefore.at(from+window-1) + from + window;
		}
	}
	(*seq) = &(seqOfActualPos[actualFrom]);
	return(actualTo-actualFrom);
}
bool LDPCStruct::addNewEdgeBetweenNativeAndLdpc(int n, int l){
	if(native2ldpc[n].find(l) != native2ldpc[n].end()){
		return false; // edge already exists
	}
	native2ldpc[n].insert(l);
	ldpc2native[l].insert(n);
	return true;
}
void LDPCStruct::unittest(int start, int wind){
	cout << "For native: "<<endl;
	for(int i = 0; i < native2ldpc.size(); i++){
		cout<<"ntv#"<<i<<":";
		for(set<int>::iterator it = native2ldpc[i].begin(); it != native2ldpc[i].end(); ++it){
			cout << *it <<",";
		}
		cout<<endl;
	}
	cout << "For LDPC: "<<endl;
	for(int i = 0; i < ldpc2native.size(); i++){
		cout<<"ldpc#"<< setfill(' ')<<setw(2)<<i;
		cout<< "(a:" << ldpcActualPos[i] <<",l:";
		cout<< setfill(' ')<<setw(3)<< ldpcLogicalPosAfter[i] << "):";
		for(set<int>::iterator it = ldpc2native[i].begin(); it != ldpc2native[i].end(); ++it){
			cout << setfill(' ') << setw(3) << *it <<",";
		}
		cout<<endl;
	}
	int *a;
	int l = getAcutalSeqWithLdpc(start,wind,&a);
	cout<<"native from:"<< start << ", window:"<< wind <<" : actual length:"<<l<<endl;
	for(int i = 0; i < l; i++){
		cout<<a[i]<<",";
	}
	cout<<endl;
}




