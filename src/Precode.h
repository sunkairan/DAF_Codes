#ifndef PRECODE_H
#define PRECODE_H

#include "PrecodeLayout.h"
#include "Utilities.h"

#include "PacketBuffer.h"

#include <vector>

using namespace std;

struct VariableNode;

class Precode {
	public:
		Precode(const PrecodeLayout& lay, int ldpcVarDegree) :
			layout(lay), ldpcVarDegree(ldpcVarDegree) {
			//Save a local copy of the parameters
			dataPacketNum = layout.GetDataPacketNum();
			ldpcNum = layout.GetldpcNum();
			hdpcNum = layout.GethdpcNum();
			additionalPerminactNum = layout.GetAdditionalPerminactNum();
			
			keyHDPC = 54896;//good for field size 2^4 and 2^8 bad for 2^2
			//keyHDPC = 12564; //good for field size 2^2 bad for 2
			psrand = new MTRand();
		}
		~Precode() {
			delete psrand;
		}

		void GenerateCheckPackets(PacketBuffer& buf);
		void GenerateHDPCConstraints(SymbolType** CH, SymbolType** YH, vector<VariableNode>& var, PacketBuffer& buf, int nInactVar);
	private:
		const PrecodeLayout& layout;
		//Local copy of parameter in PrecodeLayout
		int dataPacketNum, ldpcNum, hdpcNum;
		int additionalPerminactNum;
		

		int keyHDPC;
		MTRand* psrand;
		
		int ldpcVarDegree;
		
		void MultiplyQ(SymbolType** output, SymbolType** input, int row, bool CMP);
};

#endif /* PRECODE_H */

