#include "GBNRdtSender.h"
#include "Global.h"
#include "stdafx.h"

GBNRdtSender::GBNRdtSender() :nextseqnum(0), base(0), winNum(0), waitingState(false)
{
}

GBNRdtSender::~GBNRdtSender()
{
}

bool GBNRdtSender::getWaitingState()
{
	return this->waitingState;
}

bool GBNRdtSender::send(const Message& message)
{
	if (this->waitingState) 
	{	//sender���ڵȴ�ȷ��״̬
		return false;
	}

	if ((this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM < Configuration::WINDOWS_N)
	{	//���������δ���͵ķ���
		this->packetWindow[this->winNum].acknum = -1;																//���Ը��ֶ�
		this->packetWindow[this->winNum].seqnum = this->nextseqnum;													//��ȷ�ϺŸ�ֵ�����
		this->packetWindow[this->winNum].checksum = 0;
		memcpy(this->packetWindow[this->winNum].payload, message.data, sizeof(message.data));						//����Ҫ���͵�����
		this->packetWindow[this->winNum].checksum = pUtils->calculateCheckSum(this->packetWindow[this->winNum]);	//����У���
		pUtils->printPacket("���ͷ����ͱ���", this->packetWindow[this->winNum]);									//��ӡ���ݱ�
		if (this->base == this->nextseqnum)
		{	//û�б�nextseqnum������ѷ��͵�δ��ȷ�ϵķ��飬�������ö�ʱ��
			pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWindow[this->winNum].seqnum);
		}
		pns->sendToNetworkLayer(RECEIVER, this->packetWindow[this->winNum]);										//�ݽ������
		this->nextseqnum = (this->nextseqnum + 1) % Configuration::PACKET_NUM;										//����nextseqnum
		this->winNum++;
		if (this->winNum == Configuration::WINDOWS_N)
		{	//�����ڲ�����δ���͵ķ���
			this->waitingState = true;
			return false;
		}
		this->waitingState = false;
		return true;
	}
	else
	{	//�����ﲻ����δ���͵ķ���
		this->waitingState = true;
		return false;
	}
}

void GBNRdtSender::receive(const Packet& ackPkt)
{
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	if (checkSum != ackPkt.checksum)
	{	//���У����Ƿ���ȷ
		cout << "���ͷ��յ�����ACK��У��ͳ���" << endl;
		return;
	}

	//�ж��Ƿ��յ����ڴ����ڵ�ack
	if (this->base <= this->nextseqnum) {
		if (ackPkt.acknum < this->base || ackPkt.acknum >= this->nextseqnum)
			return;
	}
	if (this->base > this->nextseqnum) {
		if (ackPkt.acknum < this->base && ackPkt.acknum >= this->nextseqnum)
			return;
	}
	
	cout << endl << "--------------------���ͷ������ڻ���ǰ--------------------" << endl;
	for (int i = 0; i < (this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM; i++)
	{
		cout << endl << "������ţ�" << this->packetWindow[i].seqnum << "	";
		pUtils->printPacket("�������ݣ�", this->packetWindow[i]);
	}
	cout << endl << "--------------------���ͷ������ڻ���ǰ--------------------" << endl << endl;
	
	int prebase = this->base;
	for (int i = 0; i <= (ackPkt.acknum - prebase + Configuration::PACKET_NUM) % Configuration::PACKET_NUM; i++)
	{	//�ƶ����ڲ��رն�ʱ��
		pUtils->printPacket("���ͷ��յ���ȷȷ��", ackPkt);
		pns->stopTimer(SENDER, this->packetWindow[i].seqnum);
		this->base = (this->base + 1) % Configuration::PACKET_NUM;
		this->winNum--;
	}
	int j = (this->base - prebase + Configuration::PACKET_NUM) % Configuration::PACKET_NUM, i = 0;
	while (j < (this->nextseqnum - prebase + Configuration::PACKET_NUM) % Configuration::PACKET_NUM)
		this->packetWindow[i++] = this->packetWindow[j++];

	cout << endl << "--------------------���ͷ������ڻ�����--------------------" << endl;
	for (i = 0; i < (this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM; i++)
	{
		cout << endl << "������ţ�" << this->packetWindow[i].seqnum << "	";
		pUtils->printPacket("�������ݣ�", this->packetWindow[i]);
	}
	cout << endl << "--------------------���ͷ������ڻ�����--------------------" << endl << endl;

	if (this->base == this->nextseqnum) {
		//û���Ѵ��仹δ��ȷ�ϵķ���
	}
	else
	{
		for (i = 0; i < (this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM; i++)
		{	//Ϊ��ǰ�������ѷ��ͻ�û���յ�ȷ�ϵķ��鿪����ʱ��
			pns->stopTimer(SENDER, this->packetWindow[i].seqnum);
			pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWindow[i].seqnum);
		}
	}
	this->waitingState = false;
}

void GBNRdtSender::timeoutHandler(int seqNum)
{
	cout << endl << "��ʱ�ط����ش�����" << this->base << "�Լ����ı���" << endl;
	for (int i = 0; i < this->winNum; i++)
	{
		pUtils->printPacket("�ط�", this->packetWindow[i]);
		pns->stopTimer(SENDER, this->packetWindow[i].seqnum);
		pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWindow[i].seqnum);
		pns->sendToNetworkLayer(RECEIVER, this->packetWindow[i]);
	}
}
