#pragma once
#include "buffer.hpp"
#include "Network.hpp"

class Endpoint
{
protected:
	virtual uint64_t read(char* data, uint64_t size) = 0;
	virtual uint64_t readableSize() = 0;
};

class TcpEndpoint
: public Endpoint
{
private:
	RingBuffer* _buf;
public:
	TcpEndpoint():_buf(new RingBuffer(64 * 1024 * 1024)){}
	~TcpEndpoint(){
		delete _buf;
	}

	void write(const struct PacketDesc& pkt)
	{
		const DataLinkLayer::Ethernet* eth = reinterpret_cast<const DataLinkLayer::Ethernet*>(pkt.data);
		const NetworkLayer::Ip* ip = reinterpret_cast<const NetworkLayer::Ip*>(pkt.data + eth->offsetToBody());
		const TransportLayer::Tcp* tcp = reinterpret_cast<const TransportLayer::Tcp*>(pkt.data + eth->offsetToBody() + ip->offsetToBody());

		uint32_t offset = eth->offsetToBody() + ip->offsetToBody() + tcp->offsetToBody();
		uint32_t len = pkt.hdr.len - offset;

		if(len)
		{
		printf("SEQ NO=%u\n", tcp->seqNo());
			_buf->write(reinterpret_cast<const char*>(pkt.data + offset), pkt.hdr.len - offset);
		}
		else
		{
			switch (tcp->flags()) {
				case 0x2:
					printf("SYN\n");
					break;
				case 0x1:
					printf("FIN\n");
					break;
				case 0x10:
					//printf("ACK\n");
					break;
				case 0x12:
					printf("SYN ACK\n");
					break;
					break;
				case 0x4:
					printf("RST\n");
					break;
				default:
					printf("Unkown:%x\n",tcp->flags());
			}

#if 0
			printf("%x\n", tcp->flags());
			printf(/*"eth Hdr:%d, ip Hdr:%d, tcp Hdr:%d, bodyLen:%d"*/
					"URG:%x, ACK:%x, PSH:%x, RST:%x, SYN:%x, FIN:%x\n",
					/*eth->offsetToBody(), ip->offsetToBody(), tcp->offsetToBody(), len,*/
					tcp->isUrglagOn(), tcp->isAckFlagOn(), tcp->isPshFlagOn(),
					tcp->isRstFlagOn(), tcp->isSynFlagOn(), tcp->isFinFlagOn());
#endif

		}
	}

	uint64_t read(char* data, uint64_t size)
	{
		return _buf->read(data, size);
	}

	uint64_t readableSize()
	{
		return _buf->readableSize();
	}
};
