#include "../../udpserver.h"

#define TF_TYPE_BEGIN	1
#define TF_TYPE_DATA	2
#define TF_TYPE_END 	3

//计算ping-pong
#define TF_TYPE_PING 	127
#define TF_TYPE_PONG 	126


//server-client session:
class sertask :public udptask
{
public:
	/*
		@conv: client id
		@usocket:client sock
		@paddr: client addr
	*/
	sertask(IUINT32 conv, SOCKET usocket, struct sockaddr_in *paddr)
	{
		udpsock = new udpsocket(usocket, paddr);
		udptask::init(conv, udpsock);
		closeit = false;
		fp = NULL;
		nexttime = 0;
		recvsize = 0;
	}
	~sertask()
	{
		if (udpsock != NULL){
			delete udpsock;
		}
		if (fp != NULL){
			fclose(fp);
		}
	}
	virtual bool isalive()
	{
		if (closeit)
		{
			return false;
		}
		return udptask::isalive();
	}
	
	/*
		...
	*/
	virtual int parsemsg(const char *buf, int len)
	{
		if (len < 1)
		{
			printf("数据错误 %s,%d\n", buf, len);
			return -1;
		}
		switch (buf[0])
		{
		case TF_TYPE_BEGIN:
			{
				const char *filename = buf + 1;
				fp = fopen(filename, "wb");
				if (fp == NULL)
				{
					printf("创建文件失败 %s \n", filename);
					return -1;
				}
				printf("创建文件 %s \n", filename);
			}
			break;
		case TF_TYPE_DATA:
			{
				if (fp == NULL)
				{
					printf("未创建文件 %d \n", len);
					return -1;
				}
				while (fwrite(buf + 1, len - 1, 1, fp) != 1)
				{
				}
				recvsize += len;
				IUINT32 current = iclock();
				if (nexttime < current)
				{
					printf("%.2f M/S\r", float(recvsize)/(1024*1024));
					recvsize = 0;
					nexttime = current + 1000;
				}
			}
			break;
		case TF_TYPE_END:
			{
				if (fp != NULL)
				{
					fclose(fp);
				}
				printf("接收文件完成 \n");
				closeit = true;
			}
			break;
		case TF_TYPE_PING:
			{
				time_measure_t::MarkTime("TF_TYPE_PING");

				char buf2[64];
				memcpy(buf2, buf, len);
				buf2[0] = TF_TYPE_PONG;
				this->send(buf2, len);
				//printf("接收文件完成 \n");
			}
			break;
		default:
			printf("指令错误 %s,%d\n", buf, len);
		}
		return 0;
	}
private:
	udpsocket *udpsock;
	bool closeit;
	FILE *fp;
	IUINT32 nexttime;
	int recvsize;
};

int main(int argc, char *argv[])
{
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

	const char *ip = "0.0.0.0";
	int port = 9001;

	udpserver<sertask> s;
	s.bind(ip, port);

	printf("server: %s, %d\n",ip,port);

	std::chrono::milliseconds dura(10);
	for (;;)
	{
		std::this_thread::sleep_for(dura);
	}

	s.shutdown();
#ifdef _WIN32
	WSACleanup();
#endif
	return 0;
}
