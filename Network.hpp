#pragma once
#define __USE_BSD
#include <netinet/ip.h>
#define __FAVAOR_BSD
#include <netinet/tcp.h>
#include <net/ethernet.h>

#include <byteswap.h>

namespace TransportLayer
{
	struct Tcp
	{
		struct tcphdr hdr;
		//const u_char* data;
		uint32_t offsetToBody() const
		{
			return hdr.th_off * 4;
		}

		uint32_t seqNo() const
		{
			return bswap_32(hdr.th_seq);
		}

		uint8_t flags() const
		{
			return reinterpret_cast<const char*>(&hdr)[13];
		}

		bool isFinFlagOn() const
		{
			return hdr.fin;
		}

		bool isSynFlagOn() const
		{
			return hdr.syn;
		}

		bool isRstFlagOn() const
		{
			return hdr.rst;
		}

		bool isPshFlagOn() const
		{
			return hdr.psh;
		}

		bool isAckFlagOn() const
		{
			return hdr.ack;
		}

		bool isUrglagOn() const
		{
			return hdr.urg;
		}

	};
};

namespace NetworkLayer
{
	struct Ip
	{
		struct iphdr hdr;
		//const u_char* data;
		uint8_t type() const
		{
			return hdr.protocol;
		}

		uint32_t offsetToBody() const
		{
			return sizeof(struct iphdr);
		}

	};
};

namespace DataLinkLayer
{
	struct Ethernet
	{
		struct ether_header hdr;
		//const u_char* data;

		uint16_t type() const
		{
			return bswap_16(hdr.ether_type);
		}

		uint32_t offsetToBody() const
		{
			if(type() == ETHERTYPE_VLAN)
			{
				return sizeof(struct ether_header) + 4/*valn tag*/;
			}
			else
			{
				return sizeof(struct ether_header);
			}

		}
	};
}
