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
	Packet lastAckPkt;										//�ϴη��͵�ȷ�ϱ���
	RcvPacket RcvpacketWindow[Configuration::RECEIVER_N];	//���ջ��������
	int RcvWinnum;											//���մ��ڴ�С
	int rcvbase;											//���մ��ڻ����
public:
	void receive(const Packet& packet);						//���ձ��ģ�����NetworkService����
public:
	SRRdtReceiver();
	virtual ~SRRdtReceiver();
};

