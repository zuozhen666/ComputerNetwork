#pragma once
#include "RdtReceiver.h"
class TCPRdtReceiver :
	public RdtReceiver
{
private:
	int expectSequenceNumberRcvd;		//�ڴ��յ�����һ���������
	Packet lastAckPkt;					//�ϴη��͵�ȷ�ϱ���

public:
	void receive(const Packet& packet);	//���ձ��ģ�����NetworkService����

public:
	TCPRdtReceiver();
	virtual ~TCPRdtReceiver();
};

