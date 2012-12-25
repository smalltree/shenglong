#include <VxWorks.h>
#include <taskLib.h>
#include <sockLib.h>
#include <inetLib.h>
#include <ioLib.h>
#include <logLib.h>
#include <string.h>
#include <stdio.h>
#include <netinet\\tcp.h>

#define MAX_SIZE  1024
#define  IP_ADDR "10.0.0.100"
#define  PORT	7000

char tmp122[MAX_SIZE];

void sendTest(int n)
{
	int i;
	int netSkt;
	struct sockaddr_in  serverAddr;
	int sockAddrSize;
	struct timeval	 connectTimeOut;
	char optval = 1;

	/* creat socket */
	if((netSkt = socket(AF_INET, SOCK_STREAM, 0)) == ERROR){
		logMsg("socket creat failed!\n", 0,0,0,0,0,0);
		return;
	}

	/* server set */
	sockAddrSize = sizeof(struct sockaddr_in);
	bzero((char *)&serverAddr, sockAddrSize);
	serverAddr.sin_family = AF_INET;
       serverAddr.sin_len = sockAddrSize;
       serverAddr.sin_port = htons(PORT);
       serverAddr.sin_addr.s_addr = inet_addr(IP_ADDR);

	/* connect time */
	connectTimeOut.tv_sec = 2;
	connectTimeOut.tv_usec = 0;

	if((connectWithTimeout(netSkt, (struct sockaddr *)&serverAddr,
		sockAddrSize, &connectTimeOut)) == ERROR){
		logMsg("connect time out\n", 0,0,0,0,0,0);
		return;
	}
	else{
		logMsg("connect succeed\n", 0,0,0,0,0,0);
	}

	setsockopt(netSkt, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));

	for(i = 0; i < 1/*1024*32*/; i ++)
	{
		bzero(tmp122, 1024);
		sprintf(tmp122, "Loongson2F_7010A Socket Test num : em%d\n", n);
		if(send(netSkt, tmp122, 1024, 0) == ERROR){
					logMsg("send error\n", 0,0,0,0,0,0);
		}
	}
	close(netSkt);
	logMsg("send done\n", 0,0,0,0,0,0);
}
