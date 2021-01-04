#pragma once
#include "RdtSender.h"

typedef struct 
{
	Packet pkt;	//已发送并等待确认的分组
	int Ack;	//该分组是否收到确认ACK
} SRPacket;


class SRRdtSender :
	public RdtSender
{
private:
	int base;											//窗口基序号
	int nextseqnum;										//下一个发送序号
	int winNum;											//下一个发送的窗口序号
	bool waitingState;									//是否处于等待Ack的状态
	SRPacket packetWindow[Configuration::SENDER_N];		//最多允许已发送还未确认的分组窗口

public:
	bool getWaitingState();
	bool send(const Message& message);					//发送应用层下来的Message，由NetworkServiceSimulator调用,
														//如果发送方成功地将Message发送到网络层，返回true;
														//如果因为发送方处于等待正确确认状态而拒绝发送Message，则返回false
	void receive(const Packet& ackPkt);					//接受确认Ack，将被NetworkServiceSimulator调用	
	void timeoutHandler(int seqNum);					//Timeout handler，将被NetworkServiceSimulator调用

public:
	SRRdtSender();
	virtual ~SRRdtSender();
};

