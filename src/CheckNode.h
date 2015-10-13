#ifndef CHECKNODE_H
#define CHECKNODE_H

#include "Utilities.h"
#include "FiniteField.h"
#include "Partition.h"

#include "BEdge.h"
#include "VariableNode.h"

#include "InactDecoder.h"

#include <cstring>

#include "platform.h"//d_log

//With only a little constant, this is okay...
namespace vtype
{
	const int undecoded = 0;
	const int decoded = 1;
	const int inacted = 2;
}

class CheckNode {
public:
    //matrix Y_i of received packets excluding coding vector
    SymbolType** packet;
    //matrix H_i of coding vectors
    SymbolType** codingVec;
    //inactive coefficients
    SymbolType** inactCoef;
	
    // rank of the current coding vectors
    int codingRank;
    // number of received packets
    int numRec;
    
    // id of the check node 
    int id;
    
	Partition<BEdge*> edges;

	//Other parameters
    int batchSize;
    
    bool inQueue;
    bool decoded;

    bool missingRank; // default is false. if true, not count in rank distribution

    CheckNode(int id, int batchSize, int packetSize, int maxInact) :
		id(id), batchSize(batchSize), edges(3) {
        numRec = 0;
        decoded = false;
        inQueue = false;
        missingRank = false;
        
        inactCoef = mallocMat<SymbolType>(batchSize, maxInact);
        
        packet = mallocMat<SymbolType>(batchSize, packetSize);
        codingVec = mallocMat<SymbolType>(batchSize, batchSize);
        codingRank = 0;
    }

    ~CheckNode() {
		for (RangeIterator<BEdge*> it = edges.begin(0, edges.size()-1); it != edges.end(0, edges.size()-1); ++it) {
			if ((*it) != NULL)
				delete *it;
		}

		//cout << "~CheckNode: id = " << id << endl;        

        freeMat(inactCoef, batchSize);
        // free packet
        freeMat(packet, batchSize);
        freeMat(codingVec, batchSize);
    }
	
    /* Methods to manage edges */
    inline BEdge* addEdge(VariableNode * v, int size){
        BEdge* e = new BEdge(this, v, size);
        v->addEdge(e);

		//Determine the type of v and add e accordingly
		int vnodeType;
		if (!v->active())
			vnodeType = vtype::inacted;
		else if (v->decoded)
			vnodeType = vtype::decoded;
		else
			vnodeType = vtype::undecoded;
		
		edges.insert(e, vnodeType);
		//Then save its location
		e->seqInCheck = edges.size() - 1;
		
		//d_log("addEdge: var->id = %d, udeg = %d, adeg = %d, odeg = %d\n", v->id, udeg, adeg, odeg);
        return e;
    }
    
    inline void addInact(BEdge *e, int seq){
		edges.changeClass(e->seqInCheck, vtype::inacted);
        
        for (int i = 0; i < numRec; i++)
            inactCoef[i][seq] = e->gh[i];
    }
	
	/* Auxiliary methods */
    inline bool codingVecIndepend(const SymbolType *cv) {

        memcpy(codingVec[numRec], cv, batchSize);

        // check if the packet takes new information
        int hRank = FF.rankM(codingVec, numRec + 1, batchSize);

        if (hRank <= codingRank) {
            return false;
        } else {
            codingRank = hRank;
            return true;
        }
    }
	
	/* Main methods for data processing and related logic */
    inline void subsInPacket(int idx, VariableNode* var, int coef, int packetSize, int maxInact){
        FF.addvvcCMP(packet[idx], var->packet, coef, packetSize);
        FF.addvvc(inactCoef[idx], var->inactCoef, coef, maxInact);
    }
    
    inline void subsDecodedVar(VariableNode* var, BEdge* e, int packetSize, int maxInact) {
		//d_log("subsDecodedVar: var->id = %d\n", var->id);

        // substitute the value of decoded variable node into the batch
        for (int i = 0; i < numRec; i++) {
            subsInPacket(i, var, e->gh[i], packetSize, maxInact);
        }

		edges.changeClass(e->seqInCheck, vtype::decoded);
		//d_log("subsDecodedVar: var->id = %d, exchange: seqInCheck = %d, udeg = %d\n", var->id, e->seqInCheck, udeg);
    }
    
    void receivePacket(const SymbolType *packetPayload, const SymbolType *codingVec, int packetSize, int maxInact)
	{
		memcpy(packet[numRec], packetPayload, packetSize);  
		
		// update active and inactive coefficients of the packet
		VariableNode* current_vnode;
		SymbolType c;
		int i;
		
		//d_log("CheckNode: receivePacket: udeg = %d, adeg = %d, odeg = %d\n", udeg, adeg, odeg);
		
		//undecoded active variables
		for (ClassIterator<BEdge*> it = edges.beginCls(vtype::undecoded); it != edges.endCls(vtype::undecoded); ++it) {
			c = FF.innerprod((*it)->g, codingVec, batchSize);
			(*it)->gh[numRec] = c;
		}
		
		//decoded active variables
		for (ClassIterator<BEdge*> it = edges.beginCls(vtype::decoded); it != edges.endCls(vtype::decoded); ++it) {
			current_vnode = (*it)->vnode;
			c = FF.innerprod((*it)->g, codingVec, batchSize);
			subsInPacket(numRec, current_vnode, c, packetSize, maxInact);
		}
		
		//inactive variables
		for (ClassIterator<BEdge*> it = edges.beginCls(vtype::inacted); it != edges.endCls(vtype::inacted); ++it) {
			current_vnode = (*it)->vnode;
			c = FF.innerprod((*it)->g, codingVec, batchSize);
			//Note: pVar is the old name for current_vnode
			//d_log("CheckNode: receivePacket: numRec = %d, pVar->id = %d, pVar->inactSeq = %d, i = %d\n", 
			//      numRec, pVar->id, pVar->inactSeq, i);
			FF.incr(inactCoef[numRec][current_vnode->inactSeq], c);
		}
		numRec++;
	}
	
    void receivePacket_AfterBatchSolved(InactDecoder& inact, const SymbolType *packetPayload, const SymbolType *codingVec, int packetSize, int maxInact) {
		if (inact.Saturated())
			return;
		
		inact.ResetNewSlot();
		
        memcpy(inact.GetYNewSlot(), packetPayload, packetSize);
        // update active and inactive coefficients of the packet
        VariableNode* current_vnode;
        SymbolType c;

		//decoded active variables
        for (ClassIterator<BEdge*> it = edges.beginCls(vtype::decoded); it != edges.endCls(vtype::decoded); ++it) {
            current_vnode = (*it)->vnode;
            c = FF.innerprod((*it)->g, codingVec, batchSize);
            // substitute packet        
            FF.addvvcCMP(inact.GetYNewSlot(), current_vnode->packet, c, packetSize);
            // substitute inactive
            FF.addvvc(inact.GetCNewSlot(), current_vnode->inactCoef, c, maxInact);
        }

		//inactive variables
        for (ClassIterator<BEdge*> it = edges.beginCls(vtype::inacted); it != edges.endCls(vtype::inacted); ++it) {
            current_vnode = (*it)->vnode;
            c = FF.innerprod((*it)->g, codingVec, batchSize);
            FF.incr(inact.GetCNewSlot()[current_vnode->inactSeq], c);
        }
		
		if (inact.NewGlobalConstraintIndependent()) {
			inact.RegisterGlobalConstraint();
			d_log("decoder: receivePacket: decoded: increase nC2\n");
		} else {
			//Tom: Global constraint redundancy, 8 July 2013.
			//stat->AddLossGlobal();
		}
    }
	
	//Temp method
	//Assumption: checknode already solved
	void SaveGlobalConstraint(InactDecoder& inact, bool allDecoded, SymbolType** invMat, int packetSize, int maxInact)
	{
		for (int i = edges.sizeCls(vtype::undecoded); i < numRec; i++)
		{
			if (inact.Saturated())
				break;
			
			//Save to C
			if (allDecoded)
				memcpy(inact.GetCNewSlot(), inactCoef[i], maxInact);
			else
				FF.mulmcv(inact.GetCNewSlot(), inactCoef, invMat[i], numRec, maxInact);
			
			if (inact.NewGlobalConstraintIndependent()) {
				//Save to Y
				if (allDecoded)
					memcpy(inact.GetYNewSlot(), packet[i], packetSize);
				else
					FF.mulmcvCMP(inact.GetYNewSlot(), packet, invMat[i], numRec, packetSize);
				
				inact.RegisterGlobalConstraint();
			}
			else {
				//Take log??
			}
		}
	}

};

#endif /* CHECKNODE_H */

