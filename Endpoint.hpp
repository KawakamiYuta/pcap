#pragma once
#include <memory>
#include <map>
#include <cstring>
#include "buffer.hpp"
#include "Network.hpp"

class Endpoint
{
protected:
	virtual uint64_t read(char* data, uint64_t size) = 0;
	virtual uint64_t readableSize() = 0;
};

struct SHandShake
{
	int isConnect;
	uint32_t seq_server;
	uint32_t seq_client;
	uint32_t ip_server;
	uint32_t ip_client;
	uint16_t port_server;
	uint16_t port_client;
	static const int DISCONNECT=0;
	static const int CONNECTING=1;
	static const int CONNECT=2;
};


class TcpEndpoint /*: public Endpoint*/
{
private:
	//RingBuffer* _server_buf;
	SHandShake _connection;
	struct _ReceptPayload
	{
		struct SPacket
		{
			SPacket(u_char* d, uint32_t s):data(d),size(s){}
			//SPacket(const SPacket& src):data(std::move(src.data)), size(src.size);
			SPacket(SPacket&& obj): data(std::move(obj.data)),size(obj.size){}
			std::unique_ptr<u_char []> data;
			uint32_t size;
		};
		const std::string _name;
		RingBuffer* _buf;
		std::map<uint32_t, struct SPacket> _packets;
		uint32_t _seq;

		_ReceptPayload(const std::string& name):_name(name),_buf(new RingBuffer(64 * 1024 * 1024)) { }
		~_ReceptPayload(){delete _buf;}

#if 0
		void init(uint32_t seq)
		{
			_seq = seq;
		}
#endif

		void reserve(const u_char* payload, uint32_t size, uint32_t seq)
		{
			//printf("\t %s RX DATA: %u:%u\n", _name.c_str(),seq, seq+size);
			struct SPacket pkt(new u_char[size], size);
			_packets.insert(std::make_pair(seq, std::move(pkt)));
			memcpy(_packets.at(seq).data.get(), payload, size);
		}

		uint32_t commit(uint32_t seq)
		{

			if(_packets.count(seq) > 0)
			{
				//printf("\t -> ACK: seq=%u\n", seq);
				struct SPacket& pkt = _packets.at(seq);
				_buf->write((const char*)(pkt.data.get()), pkt.size);
				printf("\t %s RX DATA: %u:%u\n", _name.c_str(), seq, seq+pkt.size);
				_packets.erase(seq);
				return pkt.size;
			}
			else
			{
				printf("\t Unkown ACK\n");
				return 0;
			}
		}
	};
	_ReceptPayload _serverRxPackets;
	_ReceptPayload _clientRxPackets;
public:
	TcpEndpoint(): _connection({0}),_serverRxPackets("SERVER"), _clientRxPackets("CLIENT"){}
	~TcpEndpoint(){}

	void write(const struct PacketDesc& pkt)
	{
		const DataLinkLayer::Ethernet* eth = reinterpret_cast<const DataLinkLayer::Ethernet*>(pkt.data);
		const NetworkLayer::Ip* ip = reinterpret_cast<const NetworkLayer::Ip*>(pkt.data + eth->offsetToBody());
		const TransportLayer::Tcp* tcp = reinterpret_cast<const TransportLayer::Tcp*>(pkt.data + eth->offsetToBody() + ip->offsetToBody());

		uint32_t offset = eth->offsetToBody() + ip->offsetToBody() + tcp->offsetToBody();
		uint32_t len = pkt.hdr.len - offset;

		if(_connection.isConnect == SHandShake::DISCONNECT)
		{
			if(tcp->isSynFlagOn() && tcp->isAckFlagOn())
			{
				//printf("SYN ACK: seq=%u, ack=%u\n", tcp->seqNo(), tcp->ackNo());
				_connection.seq_client = tcp->seqNo() + 1;
			}

			else if(tcp->isSynFlagOn())
			{
				//printf("SYN: seq=%u, ack=%u\n", tcp->seqNo(), tcp->ackNo());
				_connection.seq_server = tcp->seqNo() + 1;
			}
			else if(tcp->isAckFlagOn())
			{
				//printf("ACK: seq=%u, ack=%u: EXpect:%u, %u\n", tcp->seqNo(), tcp->ackNo(), _connection.seq_client+1, _connection.seq_server+1);
				if((tcp->seqNo() == (_connection.seq_server)) && (tcp->ackNo() == (_connection.seq_client)))
				{
					_connection.ip_server = ip->dstIp();
					_connection.ip_client = ip->srcIp();
					_connection.port_server = tcp->dstPort();
					_connection.port_client = tcp->srcPort();
					printf("CONNECT:Clinet(%x:%u) Rx Sequence=%u, Server(%x:%u) Rx Sequence=%u\n"
							, _connection.ip_client, _connection.port_client, _connection.seq_client,
						   	_connection.ip_server, _connection.port_server, _connection.seq_server);
					_connection.isConnect = SHandShake::CONNECT;
				}
			}
		}
		else {
			if(len)
			{
				//printf("\tDATA: seq=%u, ack=%u\n", tcp->seqNo(), tcp->ackNo());
				if(ip->dstIp() == _connection.ip_client && tcp->dstPort() == _connection.port_client)
				{
						//_client_buf->write(reinterpret_cast<const char*>(pkt.data + offset), pkt.hdr.len - offset);
					_clientRxPackets.reserve(pkt.data + offset, pkt.hdr.len - offset, tcp->seqNo());

					//シーケンス番号が期待値と一致する場合,コミット
					if(tcp->isAckFlagOn() && tcp->seqNo() == _connection.seq_client)
					{
						//printf("\t SERVER -> CLIENT ACK:seq=%u, ack=%u\n",tcp->seqNo(), tcp->ackNo());
						uint32_t offset = _clientRxPackets.commit(tcp->seqNo());
						_connection.seq_client += offset;
					}
				}

				else if(ip->dstIp() == _connection.ip_server && tcp->dstPort() == _connection.port_server)
				{
					//_server_buf->write(reinterpret_cast<const char*>(pkt.data + offset), pkt.hdr.len - offset);
					_serverRxPackets.reserve(pkt.data + offset , pkt.hdr.len - offset, tcp->seqNo());

					if(tcp->isAckFlagOn() && tcp->seqNo() == _connection.seq_server)
					{
						//printf("\t CLIENT -> SERVER ACK:seq=%u, ack=%u\n",tcp->seqNo(), tcp->ackNo());
						uint32_t offset = _serverRxPackets.commit(tcp->seqNo());
						_connection.seq_server += offset;
					}
				}
				else
				{
				}
			}
			else if(tcp->isRstFlagOn() || tcp->isFinFlagOn())
			{
				printf("DISCONNECT\n");
				_connection.isConnect = SHandShake::DISCONNECT;
			}
			else
			{
			}
		}
	}

	RingBuffer* getClientBuffer()
	{
		return _clientRxPackets._buf;
	}

	RingBuffer* getServerBuffer()
	{
		return _serverRxPackets._buf;
	}

	uint64_t read(char* data, uint64_t size, bool isClient = true)
	{
		if(isClient)
			return _clientRxPackets._buf->read(data, size);
		else
			return _serverRxPackets._buf->read(data, size);
	}

	uint64_t readableSize(bool isClient = true)
	{
		if(isClient)
			return _clientRxPackets._buf->readableSize();
		else
			return _serverRxPackets._buf->readableSize();
	}
};
