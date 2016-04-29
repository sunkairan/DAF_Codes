#ifndef PACKETSAMPLER_H
#define PACKETSAMPLER_H

#include "Utilities.h"
#include "PrecodeLayout.h"

class PacketSampler {
	public:
		PacketSampler(const PrecodeLayout& lay) : layout(lay) {
			//Save a local copy of the parameters
			dataPacketNum = lay.GetDataPacketNum();
			ldpcNum = lay.GetldpcNum();
			hdpcNum = lay.GethdpcNum();
			additionalPerminactNum = lay.GetAdditionalPerminactNum();
			
			sampledID = new int[dataPacketNum + ldpcNum + hdpcNum];//=TotalNum
		}
		~PacketSampler() {
			delete[] sampledID;
		}
		
		void Sample(int degree, int permdeg, MTRand* psrand, PosType windowSize, double *CDF, int* actualSeq) {
		    int i;
    		// Kairan : unequal sampling
		    char *sampled = new char[windowSize];
		    double pos;
		    int lb, ub, mi;
		    memset(sampled, 0, windowSize);
		    for(i = 0; i < degree; i++) {
		        do {
		            pos = psrand->rand();
		            lb = 0;
		            ub = windowSize;
		            while(ub-lb > 1){
			            mi = lb+(ub-lb)/2;
			            if(mi == 0) {
			            	mi = 0;
			            }
			            if(pos < CDF[mi])
				            ub = mi;
			            else
				            lb = mi;
		            }
		        } while(sampled[lb]);
		        sampled[lb] = 1;
		        sampledID[i] = actualSeq[lb]; // Modified by Kairan for sliding window Raptor
		    }
		
            
			//Build incrementally and in-place at sampledID
			//Kairan : substitute original sparse part:
			//RandomCombination(dataPacketNum + ldpcNum - additionalPerminactNum, degree, psrand, 0);//Sparse part
			RandomCombination(hdpcNum + additionalPerminactNum, permdeg, psrand, degree);//Perminact part
			
			//Convert to actual IDs
			// Deleted by Kairan -- already actual IDs for sliding window Raptor codes
			//for (i = 0; i < degree; i++) {
			//	sampledID[i] = layout.SparseToActual(sampledID[i]);
			//}
			for (i = degree; i < degree + permdeg; i++) {
				sampledID[i] = layout.PerminactToActual(sampledID[i]);
			}
		}
		
		
		//Following are to-be-deprecated methods preserved for backward-compatibility
		void SampleSparsePart(int degree, MTRand* psrand) {
			RandomCombination(dataPacketNum + ldpcNum - additionalPerminactNum, degree, psrand, 0);
		}
		void SamplePerminactPart(int permdeg, MTRand* psrand) {
			RandomCombination(hdpcNum + additionalPerminactNum, permdeg, psrand, 0);
		}
		
		int* sampledID;//Dynamic array
	private:
		const PrecodeLayout& layout;
		//DONE: Layout really should be in a separate class
		//Local copy of parameters in PrecodeLayout
		int dataPacketNum, ldpcNum, hdpcNum;//Parameters for actual layouts
		int additionalPerminactNum;
		
	private:
		//Core algorithm - choose k objects out of n
		void RandomCombination(int n, int k, MTRand* psrand, int displacement) {
			int i, j;
			int* arr = sampledID + displacement;
			for (i = 0; i < n; i++)
				arr[i] = i;
			for (i = 0; i < k; i++) {
				j = psrand->randInt(n - 1 - i) + i;
				swap(arr[i], arr[j]);
			}
		}
};

#endif /* PACKETSAMPLER_H */

