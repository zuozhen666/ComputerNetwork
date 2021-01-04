#pragma once
#include "RdtReceiver.h"

typedef struct
{
	Packet rcv;
	int cache;
} RcvPacket;

class SRRdtReceiver :
	public RdtReceiver
{
private:
	Packet lastAckPkt;										//上次发送的确认报文
	RcvPacket RcvpacketWindow[Configuration::RECEIVER_N];	//接收缓存的数组
	int RcvWinnum;											//接收窗口大小
	int rcvbase;											//接收窗口基序号
public:
	void receive(const Packet& packet);						//接收报文，将被NetworkService调用
public:
	SRRdtReceiver();
	virtual ~SRRdtReceiver();
};

