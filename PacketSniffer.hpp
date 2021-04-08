#pragma once
#include <pcap.h>
#include <string>
#include <stdio.h>
#include <cstdlib>

struct PacketDesc
{
	struct pcap_pkthdr hdr;
	const u_char* data;
};

class PacketSniffer
{
	private:
		pcap_t *handle;			/* Session handle */

	public:
		PacketSniffer(const std::string& name, bool offline){
			char errbuf[PCAP_ERRBUF_SIZE];
			if(offline)
			{
				handle = pcap_open_offline(name.c_str(), errbuf);
			}
			else
			{
				handle = pcap_open_live(name.c_str(), BUFSIZ, 1, 512, errbuf);
			}

			if (handle == NULL) {
				fprintf(stderr, "Couldn't open device %s: %s\n", name.c_str(), errbuf);
			}
		}

		PacketSniffer(const std::string& name):PacketSniffer(name ,false){}

		~PacketSniffer()
		{
			pcap_close(handle);
		}

		bool setFilter(const std::string& filter_exp)
		{
			struct bpf_program fp;		/* The compiled filter */
			bpf_u_int32 mask;		/* Our netmask */
			bpf_u_int32 net;		/* Our IP */

			if (pcap_compile(handle, &fp, filter_exp.c_str(), 0, net) == -1) {
				fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp.c_str(), pcap_geterr(handle));
				return false;
			}
			if (pcap_setfilter(handle, &fp) == -1) {
				fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp.c_str(), pcap_geterr(handle));
				return false;
			}
			return true;
		}

		struct PacketDesc next()
		{
			struct PacketDesc desc;
			desc.data = pcap_next(handle, &(desc.hdr));
			//printf("recv pkt size=%d\n",desc.hdr.len);
			return desc;
		}
};
