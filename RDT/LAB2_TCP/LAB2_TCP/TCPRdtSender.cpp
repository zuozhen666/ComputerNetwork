#include "stdafx.h"
#include "Global.h"
#include "TCPRdtSender.h"

TCPRdtSender::TCPRdtSender() :nextseqnum(0), base(0), winNum(0), count(0), waitingState(false)
{
	this->dupAck[0] = -1;
	this->dupAck[1] = -2;
	this->dupAck[2] = -3;
}

TCPRdtSender::~TCPRdtSender()
{
}

bool TCPRdtSender::getWaitingState() {
	return this->waitingState;
}


bool TCPRdtSender::send(const Message& message) {
	if (this->waitingState)
	{	//���ͷ����ڵȴ�ȷ��״̬
		return false;
	}

	if ((this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM < Configuration::SENDER_N)
	{
		this->packetWindow[this->winNum].acknum = -1;																//���Ը��ֶ�
		this->packetWindow[this->winNum].seqnum = this->nextseqnum;													//��ȷ�ϺŸ�ֵ�����
		this->packetWindow[this->winNum].checksum = 0;
		memcpy(this->packetWindow[this->winNum].payload, message.data, sizeof(message.data));						//����Ҫ���͵�����
		this->packetWindow[this->winNum].checksum = pUtils->calculateCheckSum(this->packetWindow[this->winNum]);	//��������
		pUtils->printPacket("���ͷ����ͱ���", this->packetWindow[this->winNum]);									//��ӡ���ݱ�
		if (this->base == this->nextseqnum)
			pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWindow[this->winNum].seqnum);				//�������ͷ���ʱ��
		pns->sendToNetworkLayer(RECEIVER, this->packetWindow[this->winNum]);										//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
		this->nextseqnum = (this->nextseqnum + 1) % Configuration::PACKET_NUM;
		this->winNum++;
		if (this->winNum == Configuration::SENDER_N) {
			this->waitingState = true;
			return false;
		}
		this->waitingState = false;
		return true;
	}
	else
	{	//��������
		this->waitingState = true;
		return false;
	}
}

void TCPRdtSender::receive(const Packet& ackPkt) {
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	if (checkSum != ackPkt.checksum)
	{
		cout << "���ͷ��յ�ȷ�ϱ��ģ�У��ͳ���" << endl;
		return;
	}

	//�յ�����ACK�������ش�
	dupAck[this->count % 3] = ackPkt.acknum;
	if (dupAck[0] == dupAck[1] && dupAck[1] == dupAck[2])
	{
		cout << endl << "------�����ش�------" << endl << endl;
		pUtils->printPacket("���ͷ��յ�3������ACK", ackPkt);
		pUtils->printPacket("�����ش����ش�����δȷ�ϵı���", this->packetWindow[0]);
		pns->stopTimer(SENDER, this->packetWindow[0].seqnum);									//���ȹرն�ʱ��
		pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWindow[0].seqnum);			//�����������ͷ���ʱ��
		pns->sendToNetworkLayer(RECEIVER, this->packetWindow[0]);
		cout << endl << "------�����ش�------" << endl << endl;
	}
	this->count++;
	//�ж��յ���ACK�Ƿ��ڴ�����
	if (this->base <= this->nextseqnum)
	{
		if ((ackPkt.acknum < this->base) || (ackPkt.acknum >= this->nextseqnum))
			return;
	}
	if (this->base > this->nextseqnum)
	{
		if ((ackPkt.acknum < this->base) && (ackPkt.acknum >= this->nextseqnum))
			return;
	}

	int i;
	cout << endl << "--------------------���ͷ������ڻ���ǰstart--------------------" << endl << endl;
	for (i = 0; i < (this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM; i++)
	{
		printf("������ţ�%d\t", this->packetWindow[i].seqnum);
		pUtils->printPacket("�������ݣ�", this->packetWindow[i]);
	}
	cout << endl << "--------------------���ͷ������ڻ���ǰend--------------------" << endl << endl;

	//���У�����ȷ���ƶ����ڣ��رն�ʱ��
	int preBase = this->base;				//�ƶ�ǰ���ڻ����
	for (i = 0; i <= (ackPkt.acknum - preBase + Configuration::PACKET_NUM) % Configuration::PACKET_NUM; i++)
	{
		pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);
		pns->stopTimer(SENDER, this->packetWindow[i].seqnum);
		this->base = (this->base + 1) % Configuration::PACKET_NUM;
		this->winNum--;	
	}
	//�ƶ�����
	int j = (this->base - preBase + Configuration::PACKET_NUM) % Configuration::PACKET_NUM;				//δȷ�Ϸ������
	i = 0;
	while (j < (this->nextseqnum - preBase + Configuration::PACKET_NUM) % Configuration::PACKET_NUM)
		this->packetWindow[i++] = packetWindow[j++];	//�ƶ�����
	cout << endl << "--------------------���ͷ������ڻ�����start--------------------" << endl << endl;
	for (i = 0; i < (this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM; i++)
	{
		printf("������ţ�%d\t", this->packetWindow[i].seqnum);
		pUtils->printPacket("�������ݣ�", this->packetWindow[i]);
	}
	cout << endl << "--------------------���ͷ������ڻ�����end--------------------" << endl << endl;

	if (this->base == this->nextseqnum) {
	}
	else
	{
		//Ϊ��ǰ�����л�û���յ�ȷ�ϵı��Ŀ�����ʱ��
		for (i = 0; i < (this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM; i++)
		{
			pns->stopTimer(SENDER, this->packetWindow[i].seqnum);
			pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWindow[i].seqnum);
		}
	}
	this->waitingState = false;
}

void TCPRdtSender::timeoutHandler(int seqNum) {
	if (this->winNum > 0)
	{
		cout << endl << "------��ʱ�ش�------" << endl << endl;
		pUtils->printPacket("��ʱ�ش����ش�����δȷ�ϵı���", this->packetWindow[0]);
		pns->stopTimer(SENDER, this->packetWindow[0].seqnum);									//���ȹرն�ʱ��
		pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWindow[0].seqnum);			//�����������ͷ���ʱ��
		pns->sendToNetworkLayer(RECEIVER, this->packetWindow[0]);								//���·������ݰ�
		cout << endl << "------��ʱ�ش�------" << endl << endl;
	}
}
