#ifndef VARIABLENODE_H
#define VARIABLENODE_H

#include "BEdge.h"
#include "Utilities.h"

class VariableNode{
private:
	static SymbolType** inactCoef_ActualBuffer;
	static int totalNum;
public:
    int id;
    int degree;
    int inactSeq;
    BEdge *edgeHead;
    
    bool decoded;
    SymbolType* packet;
    SymbolType* inactCoef;
    
	//Call them before and after using this class
	static void SetBuffer(int totalNum, int maxInact) {
		VariableNode::totalNum = totalNum;
		VariableNode::inactCoef_ActualBuffer = mallocMat<SymbolType>(totalNum, maxInact);
	}
	static void FreeBuffer() {
		freeMat(VariableNode::inactCoef_ActualBuffer, VariableNode::totalNum);
	}
	
	//C'tor
    VariableNode(int id, SymbolType* packet, int inactSeq) : id(id), degree(0), inactSeq(inactSeq), 
		edgeHead(NULL), decoded(false), packet(packet) {
		inactCoef = VariableNode::inactCoef_ActualBuffer[id];
    }
    VariableNode(int id, SymbolType* packet) : id(id), degree(0), inactSeq(-1), 
		edgeHead(NULL), decoded(false), packet(packet) {
		inactCoef = VariableNode::inactCoef_ActualBuffer[id];
    }
    
    ~VariableNode(){
		/*
			if (packet != NULL)
				delete[] packet;
			if (inactCoef != NULL)
				delete[] inactCoef;
		*/
    }
    
    inline void addEdge(BEdge* edge){
        edge->nextInVar = edgeHead;
        edgeHead = edge;
        degree++;
    }
    
    inline bool active(){
        return inactSeq < 0;
    }
    
	/*
    inline void clearInact(){
        if (inactCoef != NULL)
            delete[] inactCoef;
        
        inactCoef = NULL;
    }
	*/
    
	void saveDecoded(CheckNode* cnode, SymbolType* coef, int packetSize, int maxInact);
	void substituteInactivePart(SymbolType** inactPkg, int numInactVar, int packetSize);
};

#endif /* VARIABLENODE_H */

