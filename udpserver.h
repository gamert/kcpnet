#ifndef __UDPSERVER_H__
#define __UDPSERVER_H__

#ifdef _WIN32
#include <winsock2.h>
#endif
#include <stdio.h>
#include <thread>
#include <mutex>
#include <map>
#include<vector>
#include "udptask.h"

/*
	job thread:
*/
template<typename T>
class handlethread
{
public:
	void init()
	{
		isstop = false;
		_thread = std::thread(std::bind(&handlethread::loop, this));
	}
	void shutdown()
	{
		isstop = true;
		_thread.join();
		_mutex.lock();
		for (std::map<IUINT32, udptask*>::iterator iter = clients.begin();
			iter != clients.end(); ++iter)
		{
			delete iter->second;
		}
		_mutex.unlock();
	}
	void loop()
	{
		//微秒
		std::chrono::microseconds dura(1);
		for (; !isstop;)
		{
			_mutex.lock();
			for (std::map<IUINT32, udptask*>::iterator iter = clients.begin();
				iter != clients.end();)
			{
				if (!iter->second->isalive())
				{
					delete iter->second;
					iter = clients.erase(iter);
				}
				else
				{
					iter->second->timerloop();
					++iter;
				}
			}
			_mutex.unlock();

			_time_measure.Update();

//			std::this_thread::sleep_for(dura);
			std::this_thread::sleep_for(std::chrono::nanoseconds(1));
		}
	}

	/*
		收到raw udp报文
	@udpsock:
	@paddr:
	@conv: the user id...
	@buff:
	@size:
	*/
	void recv(SOCKET udpsock, struct sockaddr_in *paddr, IUINT32 conv, const char *buff, int size)
	{
		udptask *pclient = NULL;
		_mutex.lock();
		std::map<IUINT32, udptask*>::iterator iter = clients.find(conv);
		if (iter == clients.end())
		{
			pclient = new T(conv, udpsock, paddr);
			clients.insert(std::map<IUINT32, udptask*>::value_type(conv, pclient));
		}
		else
		{
			pclient = iter->second;
		}
		pclient->recv(buff, size);
		_mutex.unlock();
	}
public:
	volatile bool isstop;
	std::mutex _mutex;
	std::thread _thread;
	std::map<IUINT32, udptask*> clients;
	time_measure_t _time_measure;
};


#define MAX_TDNUM 1	//线程个数..

template<typename T>
class udpserver
{
public:

	/*

	@addr:
	@port:
	*/
	bool bind(const char *addr, unsigned int short port)
	{
		if (!udpsock.bind(addr, port))
		{
			return false;
		}

		isstop = false;
		_thread = std::thread(std::bind(&udpserver::run, this));

		maxtdnum = MAX_TDNUM;
		for (unsigned int i = 0; i < maxtdnum; i++)
		{
			handlethread<T>* hthread = new handlethread<T>;
			hthread->init();
			timerthreads.push_back(hthread);
		}

		return true;
	}

	void shutdown()
	{
		isstop = true;
		udpsock.shutdown();
		_thread.join();
		for (typename  std::vector<handlethread<T>*>::iterator iter = timerthreads.begin();
			iter != timerthreads.end(); ++iter)
		{
			(*iter)->shutdown();
			delete *iter;
		}
	}

	void run()
	{
		char buff[65536] = { 0 };
		struct sockaddr_in cliaddr;
		for (; !isstop;)
		{
			socklen_t len = sizeof(struct sockaddr_in);
			int size = udpsock.recvfrom(buff, sizeof(buff), (struct sockaddr*)&cliaddr, &len);
			if (size == 0)
			{
				continue;
			}
			if (size < 0)
			{
				printf("接收失败 %d,%d \n", udpsock.getsocket(), size);
				continue;
			}
			
			time_measure_t::MarkTime("recvfrom");

			IUINT32 conv = ikcp_getconv(buff);
			timerthreads[conv % maxtdnum]->recv(udpsock.getsocket(), &cliaddr, conv, buff, size);
		}
	}

private:

	udpsocket udpsock;
	std::thread _thread;
	volatile bool isstop;
	unsigned int maxtdnum;
	std::vector<handlethread<T>*> timerthreads;
};

#endif
