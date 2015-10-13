#ifndef DECPACKETBUILDER_H
#define DECPACKETBUILDER_H

#include "IPacketBuilder.h"

#include <vector>
#include "Utilities.h"

using namespace std;

class DecPacketBuilder : public IPacketBuilder {
	public:
		DecPacketBuilder(vector<VariableNode>& var, MTRand& psrand, int batchSize) : var(var), psrand(psrand), batchSize(batchSize) { }
		void BuildPacket(int j, int sampledID[], int sparseDeg, int perminactDeg) {
			//TODO:implement me
			int i;
			for (i = 0; i < sparseDeg + perminactDeg; i++) {
				if (j == 0)
					node->addEdge(&(var[sampledID[i]]), batchSize);
				BEdge* curEdge = node->edges.get(i);
#ifdef USE_VANDERMOND
				curEdge->g[j] = vand_matrix->get(i, j);
#else
				if (i < sparseDeg)
					curEdge->g[j] = (SymbolType)(psrand.randInt(FF.size - 1));
				else
					curEdge->g[j] = (SymbolType)(psrand.randInt(FF.size - 2) + 1);
#endif
			}
		}
		void setCheckNode(CheckNode* c) { node = c; }
	private:
		//Have to be pointer to be changable
		CheckNode* node;
		vector<VariableNode>& var;
		MTRand& psrand;
		//parameters
		int batchSize;
};

#endif /* DECPACKETBUILDER_H */

