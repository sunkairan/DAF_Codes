#ifndef IPACKETBUILDER_H
#define IPACKETBUILDER_H

//TODO: Give a better name
class IPacketBuilder {
public:
	virtual void BuildPacket(int j, int sampledID[], int sparseDeg, int perminactDeg) = 0;
	virtual ~IPacketBuilder() {};
};

#endif /* IPACKETBUILDER_H */

