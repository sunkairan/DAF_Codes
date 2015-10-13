#ifndef INACTDECODER_H
#define INACTDECODER_H

#include "Precode.h"

#include "Utilities.h"
#include <cstring>
#include <vector>

using namespace std;

class InactDecoder {
	public:
		InactDecoder(Precode& precode, int numInactVar, int maxInactVar, int hdpcNum, int packetSize) :
			precode(precode), numConstraint(0), numInactVar(numInactVar), maxInactVar(maxInactVar),
			hdpcNum(hdpcNum), packetSize(packetSize) {
			Y = mallocMat<SymbolType>(maxInactVar + hdpcNum, packetSize);
			C = mallocMat<SymbolType>(maxInactVar + hdpcNum, maxInactVar);
			Cspan = mallocMat<SymbolType>(maxInactVar, maxInactVar);
		}
		~InactDecoder() {
			freeMat(Y, maxInactVar + hdpcNum);
			freeMat(C, maxInactVar + hdpcNum);
			freeMat(Cspan, maxInactVar);
		}
		
		//Getters
		SymbolType* GetYNewSlot() { return Y[maxInactVar - 1 - numConstraint]; }
		SymbolType* GetCNewSlot() { return C[maxInactVar - 1 - numConstraint]; }
		
		int GetNumInactVar() { return numInactVar; }//Feature envy in trigger for inact
		int GetNumConstraint() { return numConstraint; }
		
		//Actions
		void ResetNewSlot() { memset(GetCNewSlot(), 0, maxInactVar); memset(GetYNewSlot(), 0, packetSize); }
		void RegisterGlobalConstraint() { numConstraint++; }
		void AddInactCount() { numInactVar++; }
		
		//Check current state
		bool NewGlobalConstraintIndependent();
		bool Saturated() { return (numConstraint == numInactVar || numConstraint == maxInactVar); }
		
		bool TryInactDecode(vector<VariableNode>& var, PacketBuffer& buf, int totalNum);
		
	private:
		//Parameters
		int numConstraint, numInactVar, maxInactVar;
		
		//Local copy of foriegn parameters
		int hdpcNum, packetSize;
		
		//Main buffers
		SymbolType** Y; //Y = [Y_2 Y_1 * Q_A]
		SymbolType** C; //C = [C_2 (Q_I + C_1 * Q_A)]
		//Buffer for checking linear independence in online manner
		SymbolType** Cspan;
		
		Precode& precode;
};

#endif /* INACTDECODER_H */

