#include <thread>
#include "../../client_helper.h"

#include "../../udpclient.h"
#include <timeapi.h>

#define TF_TYPE_BEGIN	1
#define TF_TYPE_DATA	2
#define TF_TYPE_END 	3

//计算ping-pong
#define TF_TYPE_PING 	127
#define TF_TYPE_PONG 	126

//client-server session:
class clitask :public udptask
{
public:
	clitask()
	{

	}

	virtual int parsemsg(const char *buf, int len)
	{
		if (buf[0] == TF_TYPE_PONG)
		{
			std::chrono::steady_clock::time_point t1;
			memcpy(&t1, buf + 1, sizeof(t1));
			std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
			std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
			//if (time_span.count() >= 1)
			printf("收到PONG %llf\n", time_span.count());


			//auto now = get_tick_count();
			//if (preTCPRecvTime == 0)
			//{
			//	preTCPRecvTime = now;
			//}
			//auto detal = now - preTCPRecvTime;
			//preTCPRecvTime = now;
			//char temp[256];
			//snprintf(temp, sizeof(temp), "1_%d;", detal);
			//asioClient->Write(temp);

			//logs.push_back(detal);
			//if (logs.size() % 200 == 0) {
			//	for (size_t i = 0; i < logs.size(); i++)
			//	{
			//		printf("%d ", logs[i]);
			//	}
			//	printf("\n");
			//}
		}
		else
		{
			printf("收到未知数据 %s,%d\n", buf, len);
		}
		return 0;
	}
};


int test_send_file(char *filename, udpclient<clitask> &c)
{
	//char *filename = argv[2];

	FILE *fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		return -1;
	}

	char buf[10240] = { 0 };

	buf[0] = TF_TYPE_BEGIN;
	strncpy(buf + 1, filename, strlen(filename));

	c.send(buf, strlen(filename) + 1);

	buf[0] = TF_TYPE_DATA;
	for (;;)
	{
		size_t rc = fread(buf + 1, 1, sizeof(buf) - 1, fp);
		if (rc <= 0)
		{
			break;
		}
		int nret = c.send(buf, rc + 1);
		if (nret < 0)
		{
			printf("发送失败 %d\n", nret);
			break;
		}
	}
	fclose(fp);

	buf[0] = TF_TYPE_END;
	c.send(buf, 1);

	return 0;
}


void send_ping(udpclient<clitask> &c)
{
	std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();

	char buf[64] = { 0 };
	buf[0] = TF_TYPE_PING;
	memcpy(buf + 1, &t2, sizeof(t2));
	c.send(buf, sizeof(t2) + 1);
}

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		printf("%s ip 文件名\n", argv[0]);
		return -1;
	}
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
	timeBeginPeriod(1); //设置精度为1毫秒

	srand((unsigned)time(NULL));

	boost::asio::io_context io_context;
	std::shared_ptr<AsioClient> asioClient = InitAsioClient(io_context, "127.0.0.1", "3333");

	//rand
	udpclient<clitask> c;
	c.connect(argv[1], 9001, rand());

	//测试发送文件...
	//int res = test_send_file(argv[2],c);
	//if (res == -1)
	//	return -1;
	send_ping(c);

	std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
	std::chrono::milliseconds dura(1);
	while (c.isalive())
	{
		std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
		std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
		if (time_span.count() >= 10)
		{
			send_ping(c);

			t1 = t2;
		}

		std::this_thread::sleep_for(dura);
	}

	c.shutdown();
#ifdef _WIN32
	WSACleanup();
#endif
	return 0;
}

