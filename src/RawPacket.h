#ifndef RAWPACKET_H_
#define RAWPACKET_H_

#include "Utilities.h"

//TODO: Should we separate the write/read side, or find a way to do both? 19 July 2013.

class RawPacket {
	public:
		RawPacket(int nSymInHead, int nFFInSym, int fieldOrder, SymbolType maskEnc, SymbolType maskDec, int batchSize) :
			nSymInHead(nSymInHead), nFFInSym(nFFInSym), fieldOrder(fieldOrder), maskEnc(maskEnc), maskDec(maskDec), batchSize(batchSize) {
			codingVector = new SymbolType[batchSize];
		}
		~RawPacket() {
			delete[] codingVector;
		}
		
		void SetForRead(SymbolType* rawpacket) {
			rawpacketPtr = rawpacket;
			//extract id
			memcpy((SymbolType*)&id, rawpacketPtr, sizeof(KeyType));
			//extract coding vector
			int k = 0;
			SymbolType aMask;
			SymbolType* corePacket = rawpacketPtr + sizeof(KeyType);
			for (int i = 0; i < nSymInHead; i++){ // 16
				aMask = maskDec; // maskDec = 0xFF << (8-fieldOrder); = 0xFF
				for (int j = 0; j < nFFInSym; j++){ //nFFInSym = 1
					codingVector[k] = (corePacket[i] & aMask) >> (nFFInSym - 1 - j)*fieldOrder;
					aMask >>= fieldOrder;
					k++;
				}
			}
			//extract payload
			payload = rawpacketPtr + sizeof(KeyType) + nSymInHead;//TODO: other field size?
		}
		//TODO: inconsistency b/w read and write side wrt to key
		void SetForWriteWOKey(SymbolType* rawpacket) {
			rawpacketPtr = rawpacket;
			payload = rawpacketPtr + nSymInHead;
		}
		void SetHeader(int i) {
			SymbolType aMask;
			int byteNum = i / nFFInSym;
			int slotNum = i % nFFInSym;
			aMask = maskEnc >> (fieldOrder * slotNum);
			rawpacketPtr[byteNum] |= aMask;
		}
		const SymbolType* GetCodingVector() { return codingVector; }//Only valid when reading
		SymbolType* GetPayload() { return payload; }
		KeyType GetID() { return id; }//Only valid when reading
	private:
		SymbolType* rawpacketPtr;//TODO: avoid name clash?
		SymbolType* codingVector;
		SymbolType* payload;
		KeyType id;
		
		int nSymInHead;
		int nFFInSym;
		int fieldOrder;
		
		SymbolType maskDec, maskEnc;
		
		int batchSize;
};

#endif /* RAWPACKET_H_ */

