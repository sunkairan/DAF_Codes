#include "NCCoder.h"
#include <iostream>

#include "platform.h"

NCCoder* NCCoder_new(int batchSize, int packetSize) { return new NCCoder(batchSize, packetSize); }
void NCCoder_genPacket(NCCoder* recoder, SymbolType *dstPacket, SymbolType *cache, int cacheSize) { recoder->genPacket(dstPacket, cache, cacheSize); }

void NCCoder::genPacket(SymbolType *dstPacket, SymbolType *cache, int cacheSize) {
    memset(dstPacket, 0, L);
    KeyType k;
    SymbolType c;

    k = getIDFromPacket(cache);

    //d_log("NCCoder: genPacket\n");

    for (int i = 0; i < cacheSize; i++) {
        c = (SymbolType)(psrand->randInt(Q));
        if (k != getIDFromPacket(cache + L*i)) {
		cout << "Warning: inconsistent batch ID, original is " << k << endl;
		cout << "cacheSize = " << cacheSize << "  " << "L = " << L << endl;
	}


        FF.addvvcCMP(dstPacket + 2, cache+L*i+2, c, L-2);
    }

    //d_log("NCCoder: genPacket done\n");
    saveIDInPacket(dstPacket, &k);
    return;

    // initialize the *packet to zero
//    for (int i = 0 ; i< L ; i++){
//        packet[i] = 0;
//    }
//    memcpy(packet, packetHeadCur+(numCur-1)*L, L);
    /*memset(packet, 0, L);

    SymbolType c;

    for (int i = 0 ; i < numCur; i ++){
        c = (SymbolType)(psrand->randInt(Q));
        FF.addvvcCMP(packet, packetHeadCur+i*L,c,L);
    }

    return batchCur; */
}
