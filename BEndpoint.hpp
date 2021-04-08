#pragma once
#include <memory>
#include <byteswap.h>

#include "TransportProtocol.hpp"

template<typename TMsgHdr>
class ApplicationProtocol
{
private:
protected:
public:
	struct MessageDesc
	{
		TMsgHdr hdr;
		std::unique_ptr<char[]> body;
	};

private:
	std::unique_ptr<TransportProtocol> _tp;
protected:
public:
	ApplicationProtocol(TransportProtocol* tp):_tp(tp){}
	~ApplicationProtocol(){}

	struct MessageDesc readMessage()
	{
		MessageDesc msg;

		while(_tp->readableSize() < sizeof(msg.hdr)){}
		_tp->read(&(msg.hdr), sizeof(msg.hdr));

		uint64_t bodySize = bswap_32(msg.hdr.bodySize);

		msg.body = new char[bodySize];
		while(_tp->readableSize() < bodySize){}
		_tp->read(msg.body, bodySize);
		return msg;
	}
};
