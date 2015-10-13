#ifndef ENCPACKETBUILDER_H
#define ENCPACKETBUILDER_H

#include "IPacketBuilder.h"

#include "PacketBuffer.h"
#include "Utilities.h"

#include "FiniteField.h"

class EncPacketBuilder : public IPacketBuilder {
	public:
		EncPacketBuilder(PacketBuffer& buf, SymbolType** payload, MTRand& psrand, int packetSize) :
			buf(buf), payload(payload), psrand(psrand), packetSize(packetSize) { }
		void BuildPacket(int j, int sampledID[], int sparseDeg, int perminactDeg) {
			SymbolType c;
			int i;
			for (i = 0; i < sparseDeg + perminactDeg; i++) {
				SymbolType* srcPacket = buf.GetPacket(sampledID[i]);
#ifdef USE_VANDERMOND
				c = vand_matrix->get(i, j);
#else
				if (i < sparseDeg)
					c = (SymbolType)(psrand.randInt(FF.size - 1));
				else
					c = (SymbolType)(psrand.randInt(FF.size - 2) + 1);
#endif
				FF.addvvcCMP(payload[j], srcPacket, c, packetSize);
			}
		}
	private:
		PacketBuffer& buf;//TODO: how to enforce const?
		MTRand& psrand;
		SymbolType** payload;
		//parameters
		int packetSize;
};

#endif /* ENCPACKETBUILDER_H */

