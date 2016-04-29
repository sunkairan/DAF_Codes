#ifndef PRECODELAYOUT_H
#define PRECODELAYOUT_H

#include "LDPCStruct.h"

class PrecodeLayout {
	public:
		PrecodeLayout(int dataPacketNum, int ldpcNum, int hdpcNum, int additionalPerminactNum, LDPCStruct* ldpc) : dataPacketNum(dataPacketNum), ldpcNum(ldpcNum), hdpcNum(hdpcNum), additionalPerminactNum(additionalPerminactNum), ldpc(ldpc) {}

	private:
		int dataPacketNum, ldpcNum, hdpcNum;//Parameters for actual layouts
		int additionalPerminactNum;
		// Added by Kairan for sliding window Raptor codes
		LDPCStruct* ldpc;

	public:
		//Getter
		int GetDataPacketNum() const { return dataPacketNum; }
		int GetldpcNum() const { return ldpcNum; }
		int GethdpcNum() const { return hdpcNum; }
		int GetAdditionalPerminactNum() const { return additionalPerminactNum; }
		// Added by Kairan for sliding window Raptor codes
		LDPCStruct* GetLdpc() const { return ldpc; }

		//Layout Conversion Functions
		inline int SparseToActual(int sparseID) const {
			if (sparseID < dataPacketNum - additionalPerminactNum) {
				return sparseID;
			} else {
				return (sparseID + additionalPerminactNum);
			}
		}
		inline int PerminactToActual(int perminactID) const {
			if (perminactID < additionalPerminactNum) {
				return ((dataPacketNum - additionalPerminactNum) + perminactID);
			} else {
				return (dataPacketNum + ldpcNum + (perminactID - additionalPerminactNum));
			}
		}
		inline int ActualToInterlaced(int actualID) const {
			if (actualID < (dataPacketNum - additionalPerminactNum))
				return actualID;
			else if (actualID < dataPacketNum)
				return actualID + ldpcNum;
			else if (actualID < dataPacketNum + ldpcNum)
				return actualID - additionalPerminactNum;
			else
				return actualID;
		}
		inline bool inPerminactPart(int actualID) const {
			return (actualID >= (dataPacketNum - additionalPerminactNum) && actualID < dataPacketNum)
					|| (actualID >= (dataPacketNum + ldpcNum));
		}
};

#endif /* PRECODELAYOUT_H */

