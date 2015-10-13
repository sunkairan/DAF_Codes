#ifndef PACKETBUFFER_H
#define PACKETBUFFER_H

#include "Utilities.h"
#include <cstdlib>

class PacketBuffer {
	public:
		PacketBuffer(int packetNum, int checkNum, int packetSize) : packetNum(packetNum), checkNum(checkNum), packetSize(packetSize) {
			packets = (SymbolType**)malloc(packetNum*sizeof(SymbolType*));
			checkPackets = NULL;//Delay init until SetBuffer
		}
		
		~PacketBuffer() {
			if (packets != NULL)
				free(packets);
			if (checkPackets != NULL)
				freeMat(checkPackets, checkNum);
		}
		
		void SetBuffer(SymbolType* buf) {
			for (int i = 0; i < packetNum; i++) {
				packets[i] = buf + i*packetSize;
			}
			if (checkNum <= 0)
				return;
			//After setting buffer, reset checkPackets
			if (checkPackets != NULL) {
				freeMat(checkPackets, checkNum);
				checkPackets = NULL;
			}
			checkPackets = mallocMat<SymbolType>(checkNum, packetSize);
		}
		
		inline SymbolType* GetPacket(int pid) {
			return (pid < packetNum) ? packets[pid] : checkPackets[pid - packetNum];
		}
		
		//Getter
		int GetPacketSize() { return packetSize; }
		SymbolType** GetCheckPackets() { return checkPackets; }
	private:
		//Parameters
		int packetNum, checkNum, packetSize;
		//Buffer
		SymbolType** packets;
		SymbolType** checkPackets;
};

#endif /* PACKETBUFFER_H */

