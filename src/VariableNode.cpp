#include "VariableNode.h"
#include "CheckNode.h"

#include "FiniteField.h"

#include <cstring>

//Static member needs to be instantiated at exactly one cpp file
SymbolType** VariableNode::inactCoef_ActualBuffer;
int VariableNode::totalNum;

void VariableNode::saveDecoded(CheckNode* cnode, SymbolType* coef, int packetSize, int maxInact)
{
	decoded = true;
	
	// save decoded packets
	FF.mulmcvCMP(packet, cnode->packet, coef, cnode->numRec, packetSize);
	// process inactive coefficients: C1
	FF.mulmcv(inactCoef, cnode->inactCoef, coef, cnode->numRec, maxInact);
}

void VariableNode::substituteInactivePart(SymbolType** inactPkg, int numInactVar, int packetSize)
{
	if (decoded){
		FF.addvmcvCMP(packet, inactPkg, inactCoef, numInactVar, packetSize);
	} else if (!active()) {
		decoded = true;
		memcpy(packet, inactPkg[inactSeq], packetSize);
	} else {
		//Exception!!
	}
}

