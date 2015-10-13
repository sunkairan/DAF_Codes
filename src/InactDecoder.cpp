#include "InactDecoder.h"

#include "BatchDec.h"

#include "FiniteField.h"
#include "platform.h"

//Assumption: new constraint set in the new slot
bool InactDecoder::NewGlobalConstraintIndependent() {
	int rank;
	memcpy(Cspan[numConstraint], C[maxInactVar - 1 - numConstraint], maxInactVar);
	rank = FF.rankM(Cspan, numConstraint+1, maxInactVar);
	
	if (rank == numConstraint + 1) {
		return true;
	} else if (rank == numConstraint) {
		return false;
	} else {
		d_log("[ERROR] NewGlobalConstraintIndependent: inconsistent nC2 value - rank = %d, nC2 = %d\n", rank, numConstraint);
		return true;
	}
}

bool InactDecoder::TryInactDecode(vector<VariableNode>& var, PacketBuffer& buf, int totalNum) {
	bool res = false;;
	
	//First fill in the HDPC constraints
	precode.GenerateHDPCConstraints(C + maxInactVar, Y + maxInactVar, var, buf, numInactVar);
	
	/* Step 3: Solve the eqn B_I * C = Y */
	int numCol = numConstraint + hdpcNum;
	SymbolType** inactPkg = mallocMat<SymbolType>(numInactVar, packetSize);
	
	int rank = FF.GaussianSolve(inactPkg,
		C + (maxInactVar - numConstraint), numInactVar, numCol,
		Y + (maxInactVar - numConstraint), packetSize);
	
	if (rank < numInactVar) {
		cout << "InactDec: Inactive variables cannot be decoded!";
		res = false;
	} else {
		for (int j = 0; j < totalNum; j++) {
			var[j].substituteInactivePart(inactPkg, numInactVar, packetSize);
		}
		res = true;
	}
	
	//Free matrix
	freeMat(inactPkg, numInactVar);
	return res;
}

