#pragma once
#include "RdtSender.h"

class GBNRdtSender :
	public RdtSender
{
private:
	int base;										//���ڻ����
	int nextseqnum;									//��һ���������
	int winNum;										//��һ�����͵Ĵ������
	bool waitingState;								//�Ƿ��ڵȴ�Ack��״̬
	Packet packetWindow[Configuration::WINDOWS_N];	//��������ѷ��ͻ�δȷ�ϵķ��鴰��
public:
	bool getWaitingState();
	bool send(const Message& message);				//����Ӧ�ò�������Message����NetworkServiceSimulator����,
													//������ͷ��ɹ��ؽ�Message���͵�����㣬����true;
													//�����Ϊ���ͷ����ڵȴ���ȷȷ��״̬���ܾ�����Message���򷵻�false
	void receive(const Packet& ackPkt);				//����ȷ��Ack������NetworkServiceSimulator����
	void timeoutHandler(int seqNum);				//Timeout handler������NetworkServiceSimulator����
public:
	GBNRdtSender();
	virtual ~GBNRdtSender();
};

