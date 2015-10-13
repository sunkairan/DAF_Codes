#include "DelayDec.h"

#include <cassert>



/* definitions of DelayDecoder */
DelayDecoder::DelayDecoder(int K, int T, SymbolType *output):
	BatsDecoder(1, K, T, output, true, 0, K-1)
{

}

DelayDecoder::DelayDecoder(int K, int T, SymbolType *output, long evalFrom, long evalTo,int randseed):
	BatsDecoder(1, K, T, output, true, evalFrom, evalTo,randseed)
{

}

CheckNode* DelayDecoder::initNewBatch(KeyType batchID) {
	int i;

	nRecBatch = (batchID > nRecBatch)? batchID : nRecBatch;//Assume sequentially transmit batches

	CheckNode *it = new CheckNode(batchID, batchSize, packetSize, maxInact);
	((DecPacketBuilder*)(builder->GetPacketBuilder()))->setCheckNode(it);
	builder->Build(batchID,fromP,windowS,modeT);

	//d_log("decoder: initNewBatch: id = %d, udeg = %d, adeg = %d, odeg = %d, odeg - adeg = %d\n",
	//  it->id, it->udeg, it->adeg, it->odeg, it->odeg - it->adeg);

	batchSet[batchID] = it;
	return it;
}

void DelayDecoder::receivePacket(SymbolType *rawPacket) {
	receivePacket(rawPacket, true);
}

void DelayDecoder::receivePacket(SymbolType *rawPacket, bool canFinish) {

	// resolve delay-aware fountain code header
	fromP = *((PosType*)rawPacket);
	windowS = *((PosType*)(rawPacket+sizeof(PosType)));
	modeT = *((ModeType*)(rawPacket+2*sizeof(PosType)));
	SymbolType *batsRawPacket = rawPacket + 2*sizeof(PosType) + sizeof(ModeType);

	//First extract all components of the raw packet
	curPacket->SetForRead(batsRawPacket);
	KeyType batchID = curPacket->GetID();
	const SymbolType* codingVec = curPacket->GetCodingVector();
	SymbolType* payload = curPacket->GetPayload();

	CheckNode* it = batchSet[batchID];

	d_log("\ndecoder: receivePacket: batchID = %d\n", batchID);
	d_log("decoder: packetSize = %d\n", packetSize);
	nRecPkg++;

	//if a new batch is received, initialize the batch
	if (it == NULL){
		d_log("decoder: receivePacket: new batch: id = %d\n", batchID);
		it = initNewBatch(batchID);
	} else if (it->decoded && it->numRec < batchSize) {
		it->missingRank = true;
	}

	//order of the 3 if branch below cannot be changed

	//if the batch is already decoded, try to save global constraint, then return
	if (it->decoded){
		d_log("decoder: receivePacket: batch-decoded: id = %d\n", batchID);

		it->receivePacket_AfterBatchSolved(*inactDecoder, payload, codingVec, packetSize, maxInact);

		return;
	}
	//if the batch is undecoded but full, packet is useless. return
	if(it->numRec >= batchSize) {
		nloss1++;
		//Tom: another kind of income redundancy, 8 July 2013.
		//stat->AddLossIncome();
		return;
	}
	//even if it is not full, it could be dependent and hence useless. return
	if (!it->codingVecIndepend(codingVec)){
		nloss2++;
		//Tom: Income Redundancy, 20 Jun 2013.
		//stat->AddLossIncome();
		return;
	}

	//Pass all tests, save the new packet

	// receive the new packet
	nSavedPkg++;
	// save the packet in the batch
	it->receivePacket(payload, codingVec, packetSize, maxInact);

	// try to decode
	decQueue->empty();

	tryPushDecQueue(it);

	while(decQueue->isNonEmpty()){
		decodeBatch();
	}

	accumulate_packets++;
	/* Added to start inactivation righth now */
	if (canFinish) {
		if (readyForInact()) {
			inactDecoding();
		}
	}
}
