#pragma once
#include "RdtSender.h"
class TCPRdtSender :
	public RdtSender
{
private:
	int nextseqnum;									//��һ���������
	int base;										//���ڻ����
	int winNum;										//��һ�����͵Ĵ������
	bool waitingState;								//�Ƿ��ڵȴ�ACK��״̬
	Packet packetWindow[Configuration::SENDER_N];	//�����������ѷ��͵���δȷ�ϵķ��鴰��
	int dupAck[3];									//����ACK
	int count;

public:

	bool getWaitingState();
	bool send(const Message& message);				//����Ӧ�ò�������Message����NetworkServiceSimulator����,
													//������ͷ��ɹ��ؽ�Message���͵�����㣬����true;
													//�����Ϊ���ͷ����ڵȴ���ȷȷ��״̬���ܾ�����Message���򷵻�false
	void receive(const Packet& ackPkt);				//����ȷ��Ack������NetworkServiceSimulator����	
	void timeoutHandler(int seqNum);				//Timeout handler������NetworkServiceSimulator����

public:
	TCPRdtSender();
	virtual ~TCPRdtSender();
};

