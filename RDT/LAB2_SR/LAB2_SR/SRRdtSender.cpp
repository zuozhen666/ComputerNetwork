#include "stdafx.h"
#include "Global.h"
#include "SRRdtSender.h"

SRRdtSender::SRRdtSender() :nextseqnum(0), base(0), winNum(0), waitingState(false)
{
	for (int i = 0; i < Configuration::SENDER_N; i++)
		packetWindow[i].Ack = 0;	//�Ƿ��ѽ���ȷ��ACK��ʼ��
}

SRRdtSender::~SRRdtSender()
{
}

bool SRRdtSender::getWaitingState() {
	return this->waitingState;
}


bool SRRdtSender::send(const Message& message) {
	if (this->waitingState)
	{	//���ͷ����ڵȴ�ȷ��״̬
		return false;
	}

	if ((this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM < Configuration::SENDER_N)
	{
		this->packetWindow[this->winNum].pkt.acknum = -1;																	//���Ը��ֶ�
		this->packetWindow[this->winNum].pkt.seqnum = this->nextseqnum;														//��ȷ�ϺŸ�ֵ�����
		this->packetWindow[this->winNum].pkt.checksum = 0;
		this->packetWindow[this->winNum].Ack = 0;
		memcpy(this->packetWindow[this->winNum].pkt.payload, message.data, sizeof(message.data));							//����Ҫ���͵�����
		this->packetWindow[this->winNum].pkt.checksum = pUtils->calculateCheckSum(this->packetWindow[this->winNum].pkt);	//��������
		pUtils->printPacket("���ͷ����ͱ���", this->packetWindow[this->winNum].pkt);										//��ӡ���ݱ�
		if (this->base == this->nextseqnum)
			pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWindow[this->winNum].pkt.seqnum);					//�������ͷ���ʱ��
		pns->sendToNetworkLayer(RECEIVER, this->packetWindow[this->winNum].pkt);											//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
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

void SRRdtSender::receive(const Packet& ackPkt) {
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	if (checkSum != ackPkt.checksum)
	{	//У��ͳ���
		cout << "���ͷ��յ�ȷ�ϱ��ģ�У��ͳ���" << endl;
		return;
	}
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
		printf("������ţ�%d (%d)\t", this->packetWindow[i].pkt.seqnum, this->packetWindow[i].Ack);
		pUtils->printPacket("�������ݣ�", this->packetWindow[i].pkt);
	}
	cout << endl << "--------------------���ͷ������ڻ���ǰend--------------------" << endl << endl;
	i = 0;

	int preBase = this->base;																							//�ƶ�ǰ���ڻ����
	this->packetWindow[(ackPkt.acknum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM].Ack = 1;	//����ѽ���
	pUtils->printPacket("���ͷ��յ����շ���ȷ��", ackPkt);
	pns->stopTimer(SENDER, this->packetWindow[(ackPkt.acknum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM].pkt.seqnum);

	//����յ���ȷ�ϵ��ڵ�ǰ�Ļ���ţ��ƶ�����
	if (ackPkt.acknum == this->base)
	{
		this->waitingState = false;
		while (this->packetWindow[i].Ack && (i++) < Configuration::SENDER_N)
		{
			this->base = (this->base + 1) % Configuration::PACKET_NUM;
			this->winNum--;
		}
		int j = (this->base - preBase + Configuration::PACKET_NUM) % Configuration::PACKET_NUM;
		i = 0;
		while (j < (this->nextseqnum - preBase + Configuration::PACKET_NUM) % Configuration::PACKET_NUM)
			this->packetWindow[i++] = packetWindow[j++];	//�ƶ�����
		while (i < Configuration::SENDER_N)					
			this->packetWindow[i++].Ack = 0;				//���ƶ���ճ����Ĵ�������

		cout << endl << "sendbase = ACK�� �ƶ����ڣ�" << endl;
		cout << "--------------------���ͷ������ڻ�����start--------------------" << endl << endl;
		for (i = 0; i < (this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM; i++)
		{
			printf("������ţ�%d (%d)\t", this->packetWindow[i].pkt.seqnum, this->packetWindow[i].Ack);
			pUtils->printPacket("�������ݣ�", this->packetWindow[i].pkt);
		}
		cout << endl << "--------------------���ͷ������ڻ�����end--------------------" << endl << endl;
	}
	else
	{
		cout << endl << "sendbase ��= ACK�� ������Ӧ����״̬��" << endl;
		cout << "--------------------���ͷ����������ݸ��º�start--------------------" << endl << endl;
		for (i = 0; i < (this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM; i++)
		{
			printf("������ţ�%d (%d)\t", this->packetWindow[i].pkt.seqnum, this->packetWindow[i].Ack);
			pUtils->printPacket("�������ݣ�", this->packetWindow[i].pkt);
		}
		cout << endl << "--------------------���ͷ����������ݸ��º�end--------------------" << endl << endl;
	}

	if (this->base == this->nextseqnum) {
		this->waitingState = false;
	}
	else
	{
		i = 0;
		//Ϊ��ǰ�����л�û���յ�ȷ�ϵı��Ŀ�����ʱ��
		for (i = 0; i < (this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM; i++)
		{
			if (this->packetWindow[i].Ack == 0)
			{
				pns->stopTimer(SENDER, this->packetWindow[i].pkt.seqnum);
				pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWindow[i].pkt.seqnum);
			}
		}
	}
}

void SRRdtSender::timeoutHandler(int seqNum) {
	for (int i = 0; i < this->winNum; i++)
	{
		if (this->packetWindow[i].Ack == 0 && this->packetWindow[i].pkt.seqnum == seqNum)
		{
			cout << endl << "------��ʱ�ش�------" << endl << endl;
			pUtils->printPacket("��ʱ�ش�", this->packetWindow[i].pkt);
			pns->stopTimer(SENDER, this->packetWindow[i].pkt.seqnum);									//���ȹرն�ʱ��
			pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWindow[i].pkt.seqnum);			//�����������ͷ���ʱ��
			pns->sendToNetworkLayer(RECEIVER, this->packetWindow[i].pkt);								//���·������ݰ�
			cout << endl << "------��ʱ�ش�------" << endl << endl;
		}
	}
}
