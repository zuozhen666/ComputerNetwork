#pragma once
#include "RdtSender.h"
class TCPRdtSender :
	public RdtSender
{
private:
	int nextseqnum;									//下一个发送序号
	int base;										//窗口基序号
	int winNum;										//下一个发送的窗口序号
	bool waitingState;								//是否处于等待ACK的状态
	Packet packetWindow[Configuration::SENDER_N];	//允许最多存在已发送但还未确认的分组窗口
	int dupAck[3];									//冗余ACK
	int count;

public:

	bool getWaitingState();
	bool send(const Message& message);				//发送应用层下来的Message，由NetworkServiceSimulator调用,
													//如果发送方成功地将Message发送到网络层，返回true;
													//如果因为发送方处于等待正确确认状态而拒绝发送Message，则返回false
	void receive(const Packet& ackPkt);				//接受确认Ack，将被NetworkServiceSimulator调用	
	void timeoutHandler(int seqNum);				//Timeout handler，将被NetworkServiceSimulator调用

public:
	TCPRdtSender();
	virtual ~TCPRdtSender();
};

