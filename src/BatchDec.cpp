#include "BatchDec.h"
#include "platform.h"

#include <cassert>


BatsDecoder* BatsDecoder_new(int M, int K, int T, SymbolType *output){ return new BatsDecoder(M, K, T, output); }

int BatsDecoder_complete(BatsDecoder* decoder, double decRatio){ return decoder->complete(decRatio); }
int BatsDecoder_getDecoded(BatsDecoder* decoder){ return decoder->getDecodedPkg(); }
void BatsDecoder_rankDist(BatsDecoder* decoder, double* rd){ decoder->rankDist(rd); }
void BatsDecoder_receivePacket(BatsDecoder* decoder, uint8_t *batch){ decoder->receivePacket(batch); }
// Patch to expose the interface for degree distribution. Tom Lam on 11 July, 2012.
void BatsDecoder_setDegreeDist(BatsDecoder* decoder, double* degreeDist, int maxDeg){ decoder->setDegreeDist(degreeDist, maxDeg); }
void BatsDecoder_selectDegree(BatsDecoder* decoder){ decoder->selectDegree(); }

int BatsDecoder_readyForInact(BatsDecoder* decoder) 
{
	return decoder->readyForInact();
}

int BatsDecoder_inactDecoding(BatsDecoder* decoder)
{
	return decoder->inactDecoding();
}

void BatsDecoder_logRankDist(BatsDecoder *decoder)
{
	return decoder->logRankDist();
}
int BatsDecoder_numInact(BatsDecoder *decoder)
{
	return decoder->numInact();
}



/* definitions of BatsDecoder */


void BatsDecoder::tryPushDecQueue(CheckNode* it){

    if(!(it->inQueue) && (it->numRec) >= (it->edges.sizeCls(vtype::undecoded))){
	    //d_log("decoder: push batch to queue: id = %d, decoded = %d, numRec = %d, udeg = %d, adeg = %d, odeg = %d\n", 
	    //	  it->id, it->decoded, it->numRec, it->udeg, it->adeg, it->odeg);
        it->inQueue = true;
        decQueue->push(it);
    }
}

void BatsDecoder::receivePacket(SymbolType *rawPacket) {
	
	//First extract all components of the raw packet
	curPacket->SetForRead(rawPacket);
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
    if (readyForInact()) {
	    inactDecoding();
    }

}

CheckNode* BatsDecoder::initNewBatch(KeyType batchID) {
    int i;
    
    nRecBatch = (batchID > nRecBatch)? batchID : nRecBatch;//Assume sequentially transmit batches
	
    CheckNode *it = new CheckNode(batchID, batchSize, packetSize, maxInact);
	((DecPacketBuilder*)(builder->GetPacketBuilder()))->setCheckNode(it);
	builder->Build(batchID);

    //d_log("decoder: initNewBatch: id = %d, udeg = %d, adeg = %d, odeg = %d, odeg - adeg = %d\n", 
    //  it->id, it->udeg, it->adeg, it->odeg, it->odeg - it->adeg);

    batchSet[batchID] = it;
    return it;
}


void BatsDecoder::decodeBatch(){
   
    CheckNode* it = decQueue->pop();   
    it->inQueue = false;
    
    int rank = 0;
	
    //undecoded active variables
	{
		//TODO
		ClassIterator<BEdge*> iter = it->edges.beginCls(vtype::undecoded);
		int i;
    	for (i = 0; iter != it->edges.endCls(vtype::undecoded); ++iter, ++i) {
        	for (int j = 0; j < it->numRec; j++){
            	GH[j][i] = (*iter)->gh[j];
        	}
    	}
	}
    d_log("---------------------\n");
    d_log("decodeBatch: id = %d, numRec = %d, udeg = %d\n",
        it->id, it->numRec, it->edges.sizeCls(vtype::undecoded));
    
	//become decoded in the interim through vnode substitutions, postprocess and return
    if (it->edges.sizeCls(vtype::undecoded) == 0) {
		it->SaveGlobalConstraint(*inactDecoder, true, NULL, packetSize, maxInact);
		
        it->decoded = true;
        return;
    }
    
	setIdentityMatrix<SymbolType>(invMat, it->numRec);
    
    rank = FF.GaussianElimination(GH,invMat,it->numRec,it->edges.sizeCls(vtype::undecoded),it->numRec);
    
    if (rank < (it->edges.sizeCls(vtype::undecoded))){
        return;
    }
    
    // rank == degree, solve the batch!
    VariableNode* current_vnode;
    // process the decoded variables of the batch
	{
		int i = 0;
    	for(ClassIterator<BEdge*> iter = it->edges.beginCls(vtype::undecoded); iter != it->edges.endCls(vtype::undecoded); ++iter, ++i) {
        	current_vnode = (*iter)->vnode;
			
			assert(current_vnode->active());//no non-active variables
			assert(!current_vnode->decoded);//no decoded variables
        	
        	current_vnode->saveDecoded(it, invMat[i], packetSize, maxInact);
        	
        	nDecoded++;
        	if (current_vnode->id < packetNum){
            	nDecodedPkg++;
        	}

        	// added by Kairan for sliding window evaluation
        	if (current_vnode->id >= evalFrom && current_vnode->id <= evalTo){
				nDecodedPkginEval++;
				// for delay-aware sliding window, to see if decoded is in time
				if ( current_vnode->id >= fromP &&
					(windowS == 0 || current_vnode->id < fromP+windowS))
				{
					nDecodedInTime++;
				}
			}
			

        	// process the related batches
        	for (BEdge* d = current_vnode->edgeHead; d != NULL; d = d->nextInVar) {
            	if (d->cnode == it)
                	continue;
            	// d_log("substitue: batch_id = %d, var_id = %d\n", 
            	//     d->cnode->id, pVar->id);
            	d->cnode->subsDecodedVar(current_vnode, d, packetSize, maxInact);
            	
            	tryPushDecQueue(d->cnode);
        	}
    	}
	}
 
    // process remaining part
	it->SaveGlobalConstraint(*inactDecoder, false, invMat, packetSize, maxInact);
	
    it->decoded = true;
    //it->udeg = 0; // must be done in the last step
	//TODO: just a quick hack
	for (int i = 0; i < it->edges.size(); i++) {
		if (it->edges.getClass(i) == 0)
			it->edges.changeClass(i, vtype::decoded);
	}
}

bool BatsDecoder::inactDec(){
	/* Dynamic inactivation */
    bool res = true;
	
	int permNum = hdpcNum + additionalPerminactNum;
    d_log("start inactivation\n");
    d_log("------ before ------\n");
    d_log("inactDec: piNum = %d, nInactVar = %d, maxInact = %d, nDecoded = %d, nDecoded + nInactVar = %d, totalNum = %d\n",
		permNum, inactDecoder->GetNumInactVar(), maxInact, nDecoded, nDecoded + inactDecoder->GetNumInactVar(), totalNum);
	
    while (res && inactDecoder->GetNumInactVar() < maxInact && (nDecoded + inactDecoder->GetNumInactVar() < totalNum)) {
		res = addInact();//inactivate a variable
    }
    
    d_log("------ after ------\n");
	d_log("inactDec: piNum = %d, nInactVar = %d, maxInact = %d, nDecoded = %d, nDecoded + nInactVar = %d, totalNum = %d\n",
		permNum, inactDecoder->GetNumInactVar(), maxInact, nDecoded, nDecoded + inactDecoder->GetNumInactVar(), totalNum);
    d_log("InactDec: nRecPkg - loss = %d, nSavedPkg = %d, loss(C2) = %d\n", nRecPkg - nloss1-nloss2, nSavedPkg, nloss3);
    d_log("nC2 = %d, hdpcNum = %d, nC2 + hdpcNum = %d\n", inactDecoder->GetNumConstraint(), hdpcNum, inactDecoder->GetNumConstraint() + hdpcNum);
	
    // if (nDecoded + nInactVar < totalNum || nC2 + hdpcNum < nInactVar) {
    if (nDecoded + inactDecoder->GetNumInactVar() < totalNum ||
		inactDecoder->GetNumConstraint() < inactDecoder->GetNumInactVar()) {
	    d_log("inactDec: fail before solving LE: nDecoded + nInactVar = %d, totalNum = %d, nC2 + hdpcNum = %d, nInactVar = %d\n",
		  nDecoded + inactDecoder->GetNumInactVar(), totalNum, inactDecoder->GetNumConstraint() + hdpcNum, inactDecoder->GetNumInactVar());
	    d_log("InactDec: nDecoded = %d, nInactVar = %d, nC2 = %d, hdpcNum = %d\n", 
		  nDecoded, inactDecoder->GetNumInactVar(), inactDecoder->GetNumConstraint(), hdpcNum);

        return false;
    }

    /* try to solve the inactive variables */
	return inactDecoder->TryInactDecode(var, *buf, totalNum);
}

bool BatsDecoder::addInact(){
    //First find a variable node for inactivation
	
	/*
	 * Choose a cnode that is the closest to being decodable in the sense of having
	 * the smallest "rank deficiencies".
	 */
	int min_Gap = 10000;
	int best_cnode_Deg = -1;
	int best_cnode_ID = -1;
	
    int cur_Gap;
	CheckNode* cur_cnode;
    
    for (int i = 0; i < nRecBatch; i++){
        cur_cnode = batchSet[i];
        if (cur_cnode != NULL && !cur_cnode->decoded){
            cur_Gap = -(cur_cnode->numRec - cur_cnode->edges.sizeCls(vtype::undecoded));
            if (cur_Gap < min_Gap || (cur_Gap == min_Gap && cur_cnode->edges.sizeCls(vtype::undecoded) > best_cnode_Deg)) {//Tie-breaker by degree
                min_Gap = cur_Gap;
                best_cnode_Deg = cur_cnode->edges.sizeCls(vtype::undecoded);
                best_cnode_ID = i;
            }
        }
    }
    
    if (best_cnode_ID < 0 || best_cnode_Deg <= 0) {
        return false;
    }
    
    /*
	 * Choose a vnode among the undecoded nodes of the chosen cnode to inactivate.
	 * While they are all equal from the perspective of the chosen cnode, they could
	 * also affect other cnodes potentially.
	 * Hence, we use the degree of a vnode as heuristic - the higher the degree, the
	 * more potential good it can do.
	 */
	
	CheckNode* best_cnode;
    VariableNode* best_vnode;
    int max_vDeg = -1;
    
    best_cnode = batchSet[best_cnode_ID];
    //cout << "addInact: best_cnode_ID = " << best_cnode_ID << endl;

    for (ClassIterator<BEdge*> iter = best_cnode->edges.beginCls(vtype::undecoded); iter != best_cnode->edges.endCls(vtype::undecoded); ++iter) {
	    // added by huming for retry
	    if ((*iter)->vnode->decoded == true)
		    continue;

        if((*iter)->vnode->degree > max_vDeg){
            best_vnode = (*iter)->vnode;
            max_vDeg = (*iter)->vnode->degree;
        }
    }
    
    //Inactivate the selected vnode
    best_vnode->inactSeq = inactDecoder->GetNumInactVar(); 
	inactDecoder->AddInactCount();
    //cout << "addInact: vnode->id = " << best_vnode->id << endl;

    d_log("addInact: var->id = %d, var->inactSeq = %d\n", best_vnode->id, best_vnode->inactSeq);

    if (best_vnode->decoded == true)
	    d_log("addInact: vnode decoded\n");
    else
	    d_log("addInact: vnode undecoded or inactive\n");

    decQueue->empty();//Reset
	
    //Register the inacted variable in all affected batches, and push to queue
    for (BEdge* d = best_vnode->edgeHead; d != NULL; d = d->nextInVar) {
	    //added by huming for retry
	    if (d->cnode->decoded == true)
		    continue;

        d->cnode->addInact(d, best_vnode->inactSeq);
        tryPushDecQueue(d->cnode);
    }
	
	//Do a decoding cycle again
    while(decQueue->isNonEmpty()){
        d_log("addInact: decodeBatch\n");
        decodeBatch();
    }

    d_log("addInact: about to return\n");
    return true;
}


void BatsDecoder::rankDist(double* rd) {
    int dd[batchSize + 1];
    double numRec = 0;
    for (int j = 0; j <= batchSize; j++) {
        dd[j] = 0;
    }
    CheckNode * it;
    for (int i = getSmallestBid(); i < nRecBatch; i++) {
        it = batchSet[i];
        if (it != NULL) {
            if (!(it->missingRank)) {
                numRec += 1;
                dd[it->codingRank]++;
            }
        }
    }

    cout << endl;

    for (int j = 0; j <= batchSize; j++) {
        rd[j] = dd[j] / (double) numRec;
    }
}

void BatsDecoder::logRankDist() {
#ifdef RANK_DEBUG

	d_printf("Rank Distribution");

	double rd[batchSize + 1];
	double sum = 0.0;
	rankDist(rd); 

	double Erk = 0.0;
    
	ofstream myfile;
	myfile.open("rankDistRuntime.txt", ios::out | ios::app);
	for (int i = 0; i <= batchSize; i++) {
		cout << rd[i] << " ";
		myfile << rd[i] << "\t";
		Erk += i * rd[i];
		sum += rd[i];
	}
	myfile << "\n";
	myfile.close();

	d_printf("E[rank(H)] = %f, sum = %f\n", Erk, sum);

#endif
}

