#ifndef BATCHBUILDER_H
#define BATCHBUILDER_H

#include "PacketSampler.h"
#include "IPacketBuilder.h"
#include "Utilities.h"

#include <vector>
#include <string>
#include <algorithm>
#include <cstdlib>

using namespace std;

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

class BatchBuilder {
	public:
		BatchBuilder(const PrecodeLayout& lay,
					int maxSparseDegree,
					int perminactDegree,
					int batchSize,
					int randseed) :
		sampler(lay),
		psrand(),
		maxSparseDegree(maxSparseDegree),
		perminactDegree(perminactDegree),
		batchSize(batchSize),
		randseed(randseed){
			packBuilder = NULL;//Set separately
			dist = NULL;//Set separately
		}
		void Build(int batchID, PosType startPos=0, PosType windowSize=0, ModeType mode=0) {
			/* Step 1: Sample the degrees */
			uint32 seeds[3] = {0};
			seeds[0] = batchID;
			seeds[1] = randseed;
			psrand.seed(seeds, 2);
			int degree = (int)(dist->sample(psrand.rand()));
			degree = min(degree, maxSparseDegree);
			// added by Kairan for delay-aware fountain code
			startPos = MIN(maxSparseDegree - 1, startPos);
			windowSize = ComputeWindowSize(startPos, windowSize);
			degree = MIN(degree, windowSize);
			
			/* Step 2: Sample the packets */
			// modified by Kairan for delay-aware fountain code
		    double *CDF = ComputeCdf(windowSize, mode);
			sampler.Sample(degree, perminactDegree, &psrand, startPos, windowSize, CDF);
			
			/* Step 3: invoke packet builder */
			for (int j = 0; j < batchSize; j++) {
				packBuilder->BuildPacket(j, sampler.sampledID, degree, perminactDegree);
			}
		}
		~BatchBuilder() {
			delete dist;
		}
		
		void SetPacketBuilder(IPacketBuilder* builder) { packBuilder = builder; }
		IPacketBuilder* GetPacketBuilder() { return packBuilder; }
		
		//Kairan : compute CDF from mode number

		double *ComputeCdf(PosType windowSize, ModeType mode) {
			// mode = 0 : uniform distribution
			// mode = -1 ~ 1 : the slope of the distribution
			PosType i;
			double *CDF = new double[windowSize+1];
			double s = (double)windowSize;
			double a = 2.0*mode / s / s;
			double b = (1-mode) / s;
			if (mode > 1.0) {
				mode = 1.0;
			}
			else if (mode < -1.0) {
				mode = -1.0;
			}
			CDF[0] = 0.0;
			// Compute CDF
			for(i = 1; i <= windowSize; i++) {
				CDF[i] = CDF[i-1] + a*((double)i - 0.5) + b;
			}

			return CDF;
		}


		/*double *ComputeCdf(PosType windowSize, ModeType mode) {
    		// mode = 0 : uniform distribution
		    // mode = 1 ~ windowSize : which is the highest point
		    PosType i;
		    double *CDF = new double[windowSize+1];
		    CDF[0] = 0.0;
		    // Compute CDF
		    if(mode == 0) {
		        // uniform distribution
		        double w = (double) windowSize;
		        for(i = 1; i <= windowSize; i++) {
		            CDF[i] = ((double)i)/ w;
		        }
		    }
		    else {
		        // mode is the highest bar
		        double wm = (double) (windowSize * mode);
		        for(i = 1; i <= mode; i++) {
		            double is = (double) (i*i);
		            CDF[i] = is / wm;
		        }
		        double wr = (double) (windowSize * (windowSize - mode));
		        for( ; i <=windowSize; i++) {
		            double is = (double) ((windowSize - i)*(windowSize - i));
		            CDF[i] = 1 - is / wr;
		        }
		    }
            return CDF;
		}*/
		
		//Kairan : compute real window size
		int ComputeWindowSize(PosType startPos, PosType windowSize){
		    if(windowSize == 0) {
		        windowSize = maxSparseDegree;
		    }
		    return( min(windowSize, maxSparseDegree - startPos) );
		}
		
		void SetDegreeDist(DistSampler* mydist) {
			if (dist != NULL)
				delete dist;
			dist = mydist;
		}
		void SetDegreeDist(double degDist[], int maxDeg) {
			if (dist != NULL)
				delete dist;
			dist = new DistSampler(degDist, maxDeg);
		}
		void SetDegreeDist(string filename) {
			vector<double>* content;
			double* degreeDist;

			content = ReadFileDouble(filename);
			degreeDist = (double*)malloc(sizeof(double)*(content->size() + 1));
			degreeDist[0] = 0.0;
			copy(content->begin(), content->end(), &(degreeDist[1]));
			
			SetDegreeDist(degreeDist, content->size() + 1);
			free(degreeDist);
			delete content;
		}
		
		//Needed to construct PacketBuilder... any simpler method?
		MTRand& GetRand() { return psrand; }
	private:
		IPacketBuilder* packBuilder;
		PacketSampler sampler;
		MTRand psrand;
		DistSampler* dist;
		int randseed;
		//parameters
		int maxSparseDegree, perminactDegree;
		int batchSize;
};

#endif /* BATCHBUILDER_H */

