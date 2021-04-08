#include <chrono>
#include <thread>

#include "PacketSniffer.hpp"
#include "CEndpoint.hpp"

void dump(char* data, uint64_t size)
{
	printf("dump %d\n", size);
	for(uint64_t i = 0; i < size; i++)
	{
		printf("%02x", data[i]);
		if((i % 4) == 3)
			printf(" ");
		if((i % 16) == 15)
			printf("\n");
	}
	printf("\n");
}

static TcpEndpoint payload;

int main(int argc, char *argv[])
{
	std::thread producer([&](){
			PacketSniffer ps("lo");
			ps.setFilter("");

			while(true)
			{
			auto pkt = ps.next();
			if(pkt.data)
			{
			payload.write(pkt);
			}
			}});

	while(true)
	{
		char buf[1024];
		uint64_t size =  payload.read(buf, 1024);
		if(size > 0)
		{
			dump(buf, size);
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	return 0;
}

