#include "Precode.h"

#include "FiniteField.h"
#include "MersenneTwister.h"

#include "BatchDec.h"//TODO: Need to separate vnode class later

void Precode::MultiplyQ(SymbolType** output, SymbolType** input, int row, bool CMP) {
	//Note: the input matrix is vectorized as many col vectors to save one explicit loop
	int Qrow = dataPacketNum + ldpcNum;
	int Qcol = hdpcNum;
	
	int deg = 2;
	SymbolType* accum = new SymbolType[row];
	SymbolType b;
	
	psrand->seed(keyHDPC);
	
	//Initialize output to zero
	for (int k = 0; k < Qcol; k++) {
		memset(output[k], 0, row);
	}
	//Initial state for accumulator
	if (input[0]!=NULL){ //Lazy way to denote all-zero column by NULL-ptr
		memcpy(accum, input[0], row);
	} else {
		memset(accum, 0, row);
	}

	//TODO: extract yet another sampling method?
	int idVec[Qcol];
	for (int i = 0; i < Qcol; i++){
		idVec[i] = i;
	}
	for (int k = 1; k < Qrow; k++) {
		for (int i = 0; i < deg; i++) {
			int j = psrand->randInt(Qcol - 1 - i) + i;
			swap(idVec[i], idVec[j]);
			
			//Add contribution at selected column
			FF.addvv(output[idVec[i]], accum, row);
		}
		//Shift and add accumulator
		(CMP) ? FF.mulvcCMP(accum, 2, row) : FF.mulvc(accum, 2, row);
		if (input[k]!=NULL){
			FF.addvv(accum, input[k], row);
		}
	}
	
	//Last row of MT; use end value of accumulator
	b = 1;
	for (int k = 0; k < Qcol; k++) {
		(CMP) ? FF.addvvcCMP(output[k], accum, b, row) : FF.addvvc(output[k], accum, b, row);
		b = FF.mul(b, 2);
	}
	
	delete[] accum;
}

void Precode::GenerateCheckPackets(PacketBuffer& buf) {
	//Layout parameters
	int packetAndLDNum = dataPacketNum + ldpcNum;
	int activeBoundary = dataPacketNum - additionalPerminactNum;
	int perminactNum = hdpcNum + additionalPerminactNum;
	
	//Handle to LDPC and HDPC part of the input buffer
    SymbolType** ldpcBuf = buf.GetCheckPackets();
    SymbolType** hdpcBuf = buf.GetCheckPackets() + ldpcNum;
	
	int packetSize = buf.GetPacketSize();
	
	//Interlaced and rearrangable handle to input buffer or matrix; used as input to MultiplyQ
    SymbolType** tmp = (SymbolType**)malloc(packetAndLDNum*sizeof(SymbolType*));
	
	/*
	 * Note: The larger block matrices/vectors, Q, C, and B are partitioned
	 * by the different part of input vector: active (A), inactive (I), ldpc (L),
	 * and hdpc (H). Inactive here refers to additionally perm-inactivated nodes
	 * in the source packet part.
	 * The subscript notation is used, so Q_A means the active part of Q matrix.
	 */
	
    //matrix for C_H
    SymbolType** CH = mallocMat<SymbolType>(ldpcNum, hdpcNum);
    //matrix for I + C_H * Q_L
    SymbolType** AH = mallocMat<SymbolType>(hdpcNum, hdpcNum);
    //matrix for [B_A B' B_I] * Q
    SymbolType** BQ = mallocMat<SymbolType>(hdpcNum, packetSize);
	
    /* Step 1: add contribution of B' = B_A * C_A + B_I * C_I to LDPC buffer, and compute C_H */

    //add contribution of B_A * C_A to LDPC buffer
    for (int k = 0; k < activeBoundary; k++) {
        for (int d = 0; d < ldpcVarDegree; d++) {
            FF.addvv(ldpcBuf[(k % ldpcNum + d * (int)(k / ldpcNum) + d) % ldpcNum], buf.GetPacket(k), packetSize);
        }
    }
    //add contribution of B_I * C_I to LDPC buffer
    for (int j = 0; j < ldpcNum; j++){
        for (int i = 0; i < 2; i++){
            int k = (j+i) % perminactNum;//The concat-along-col matrix [C_I; C_H] is circulent
            if (k < additionalPerminactNum){
                FF.addvv(ldpcBuf[j],buf.GetPacket(activeBoundary+k),packetSize);
            } else {
                CH[j][k-additionalPerminactNum] = 1;//as side effect, save coefficient of C_H
            }
        }
    }
	//Notice that now LDPC store B' exactly
	
	
	/* Step 2: Compute the equation governing HDPC part */
	
    //Compute the right hand side, BQ = [B_A B' B_I] * Q
    for (int i = 0; i < packetAndLDNum; i++){
		//DONE: getPkgHead method inlined here, what to do next?
        tmp[layout.ActualToInterlaced(i)] = buf.GetPacket(i);//Interlace B_A, B_I, and B' (which is LDPC buffer currently)
    }
    MultiplyQ(BQ, tmp, packetSize, true);
	
    //Compute the left hand side, AH = I + C_H * Q_L
    for (int i = 0; i < packetAndLDNum; i++){
        if (i < dataPacketNum)
            tmp[layout.ActualToInterlaced(i)] = NULL;//Zero-out active and inactive part
        else
            tmp[layout.ActualToInterlaced(i)] = CH[i-dataPacketNum];//Match LDPC part with C_H
    }
    MultiplyQ(AH, tmp, hdpcNum, false);
    //add identity matrix
    for(int i = 0; i < hdpcNum; i++){
        FF.incr(AH[i][i]);
    }
	
	
    /* Step 3: solve H * AH = BQ for HDPC */
    
    int rank_AH = FF.GaussianSolve(hdpcBuf, AH, hdpcNum, hdpcNum, BQ, packetSize);
    if (rank_AH < hdpcNum){
        cout << "Fatal encoding error: Cannot generate HDPC packets!" << endl;
    }
	
	
	/* Step 4: solve L = H * C_H + B' for LDPC */
	
	//LDPC already has B'; add contribution of H * C_H
    for (int i = 0; i < ldpcNum; i++){
        FF.addvmcvCMP(ldpcBuf[i], hdpcBuf, CH[i], hdpcNum, packetSize);
    }
    
    // free allocated memory
    free(tmp);
    freeMat(CH, ldpcNum);
    freeMat(AH, hdpcNum);
    freeMat(BQ, hdpcNum);
}

void Precode::GenerateHDPCConstraints(SymbolType** CH, SymbolType** YH, vector<VariableNode>& var, PacketBuffer& buf, int nInactVar) {
	//TODO: comment needs updating
	int packetAndLDNum = dataPacketNum + ldpcNum;
	
	//Input to MultiplyQ()
	SymbolType** tmp = (SymbolType**)malloc((dataPacketNum + ldpcNum)*sizeof(SymbolType*));
	
	/* Step 1: Compute Y = [Y_2 Y_1 * Q_A], with Y_2 already stored */
	for (int i = 0; i < packetAndLDNum; i++){
		//DONE: getPkgHead method inlined here, what to do next?
		if (var[i].decoded)
			tmp[layout.ActualToInterlaced(i)] = buf.GetPacket(i);
		else
			tmp[layout.ActualToInterlaced(i)] = NULL;
	}
	MultiplyQ(YH, tmp, buf.GetPacketSize(), true);//First maxC2 col are Y_2
	
	/* Step 2: Compute C = [C_2 (Q_I + C_1 * Q_A)], with C_2 already stored */
	SymbolType** IM = newIdentityMat<SymbolType>(nInactVar);
	
	//Q_I + C_1 * Q_A = [C_1 I] * Q'
	for (int i = 0; i < packetAndLDNum; i++){
		if (var[i].decoded)
			tmp[layout.ActualToInterlaced(i)] = var[i].inactCoef;
		else
			tmp[layout.ActualToInterlaced(i)] = IM[var[i].inactSeq];
	}
	MultiplyQ(CH, tmp, nInactVar, false);
	//Manually add the hdpc part of Q_I
	for (int i = 0; i < hdpcNum; i++){
		FF.incr(CH[i][var[packetAndLDNum+i].inactSeq]);
	}
	
	freeMat(IM, nInactVar);
	free(tmp);
}

