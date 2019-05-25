#ifndef __UDPTASK_H__
#define __UDPTASK_H__

#include <stdio.h>
#include "ikcp.h"
#include "common.h"
#include "udpsocket.h"

class udptask
{
public:
	udptask()
	{
		conv = 0;
		kcp = NULL;
		current = 0;
		nexttime = 0;
		alivetime = nexttime + 10000;
		memset(buffer, 0, sizeof(buffer));
	}

	~udptask()
	{
		if (kcp != NULL)
		{
			ikcp_flush(kcp);
			ikcp_release(kcp);
		}
		printf("�ر����� %d\n", conv);
	}

	bool init(IUINT32 conv, udpsocket *udpsock, int mode = 2)
	{
		if (udpsock == NULL)
		{
			return false;
		}
		this->conv = conv;
		this->nexttime = 0;

		kcp = ikcp_create(conv, (void*)udpsock);

		kcp->output = &udptask::udp_output;
		//kcp->logmask = 0xffff;
		//kcp->writelog = &udptask::writelog;

		ikcp_wndsize(kcp, 128, 128);

		switch (mode)
		{
		case 0:
			// Ĭ��ģʽ
			ikcp_nodelay(kcp, 0, 10, 0, 0);
			break;
		case 1:
			// ��ͨģʽ���ر����ص�
			ikcp_nodelay(kcp, 0, 10, 0, 1);
			break;
		case 2:
			// ��������ģʽ
			// �ڶ������� nodelay-�����Ժ����ɳ�����ٽ�����
			// ���������� intervalΪ�ڲ�����ʱ�ӣ�Ĭ������Ϊ 10ms
			// ���ĸ����� resendΪ�����ش�ָ�꣬����Ϊ2
			// ��������� Ϊ�Ƿ���ó������أ������ֹ
			//ikcp_nodelay(kcp, 0, 10, 0, 0);
			//ikcp_nodelay(kcp, 0, 10, 0, 1);
			//ikcp_nodelay(kcp, 1, 10, 2, 1);
			ikcp_nodelay(kcp, 1, 5, 1, 1); // ���ó�1��ACK��Խֱ���ش�, ������Ӧ�ٶȻ����. �ڲ�ʱ��5����.

			kcp->rx_minrto = 10;
			kcp->fastresend = 1;
			break;
		default:
			printf("%d,%d ģʽ����!\n", conv, mode);
		}

		printf("�½����� %d\n", conv);
		return true;
	}

	int recv(const char  * buf, int len)
	{
		int nret = ikcp_input(kcp, buf, len);
		if (nret == 0)
		{
			nexttime = iclock();
			alivetime = nexttime + 10000;
		}
		return nret;
	}

	int send(const char  * buf, int len)
	{
		int nret = ikcp_send(kcp, buf, len);
		if (nret == 0)
		{
			nexttime = iclock();
			alivetime = nexttime + 10000;
		}
		//printf("�������� %d,%d,%d\n", conv, len, nret);
		return nret;
	}

	void timerloop()
	{
		current = iclock();

		if (nexttime > current)
		{
			return;
		}

		nexttime = ikcp_check(kcp, current);
		if (nexttime != current)
		{
			return;
		}

		ikcp_update(kcp, current);

		while (true) {
			int nrecv = ikcp_recv(kcp, buffer, sizeof(buffer));
			if (nrecv < 0)
			{
				if (nrecv == -3)
				{
					printf("buffer̫С %d,%d\n", conv, sizeof(buffer));
				}
				break;
			}
			parsemsg(buffer, nrecv);
		}
	}

public:
	virtual bool isalive()
	{
		return alivetime > current;
	}
	virtual int parsemsg(const char *buf, int len) = 0;

private:
	static int udp_output(const char *buf, int len, ikcpcb *kcp, void *user)
	{
		return ((udpsocket*)user)->sendto(buf, len);
	}

	static void writelog(const char *log, struct IKCPCB *kcp, void *user)
	{
		printf("%s\n", log);
	}

private:
	IUINT32 conv;
	ikcpcb *kcp;
	IUINT32 nexttime;
	IUINT32 current;
	IUINT32 alivetime;
	char buffer[65536];
};

#endif
