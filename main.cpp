#include <chrono>
#include <thread>

#include "PacketSniffer.hpp"
#include "Endpoint.hpp"

void dump(char* data, uint64_t size)
{
	//printf("dump %d\n", size);
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

struct SMsg
{
	uint32_t msgId;
	uint32_t bodySize;
};

static TcpEndpoint payload;

template<typename TMsgHdr>
struct Consumer
{
	RingBuffer* _buf;
	Consumer(RingBuffer* buf):_buf(buf){}
	void operator()(){
		while(true)
		{
			while(_buf->readableSize() < sizeof(TMsgHdr)){
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			TMsgHdr hdr;
			_buf->read(reinterpret_cast<char*>(&hdr), sizeof(TMsgHdr));

			uint32_t bodySize = bswap_32(hdr.bodySize);

			char* body = new char[bodySize];
			while(_buf->readableSize() < bodySize){ }

			uint64_t size =  _buf->read(body, bodySize);

			dump(reinterpret_cast<char*>(&hdr), sizeof(TMsgHdr));

			dump(body, size);
		}
	}
};

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

	Consumer<SMsg> client(payload.getClientBuffer());
	Consumer<SMsg> server(payload.getServerBuffer());

	std::thread serverDataConsumer([&](){
			server();
			});

	std::thread clientDataConsumer([&](){
			client();
			});

	serverDataConsumer.join();
	clientDataConsumer.join();

	return 0;
}

