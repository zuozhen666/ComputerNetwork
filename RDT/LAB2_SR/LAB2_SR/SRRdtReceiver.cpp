#include "stdafx.h"
#include "Global.h"
#include "SRRdtReceiver.h"


SRRdtReceiver::SRRdtReceiver()
{
	lastAckPkt.acknum = -1; //��ʼ״̬�£��ϴη��͵�ȷ�ϰ���ȷ�����Ϊ-1��ʹ�õ���һ�����ܵ����ݰ�����ʱ��ȷ�ϱ��ĵ�ȷ�Ϻ�Ϊ-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//���Ը��ֶ�
	this->rcvbase = 0;
	this->RcvWinnum = 0;
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
		lastAckPkt.payload[i] = '.';
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
	for (int i = 0; i < Configuration::RECEIVER_N; i++)
	{
		this->RcvpacketWindow[i].cache = 0;
		this->RcvpacketWindow[i].rcv = lastAckPkt;
	}
}


SRRdtReceiver::~SRRdtReceiver()
{
}

void SRRdtReceiver::receive(const Packet& packet) {
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(packet);
	int i = 0;

	//���У�����ȷ��ͬʱ�յ����ĵ��������
	if (checkSum == packet.checksum) {
		//���������[rcvbase, rcvbase+N-1]ʱ����ACK�����汨�ģ�����Ǹô���Ϊ�ѻ���
		int a, b, c;
		a = this->rcvbase <= (this->rcvbase + Configuration::RECEIVER_N) % Configuration::PACKET_NUM;
		b = packet.seqnum >= this->rcvbase && packet.seqnum < (this->rcvbase + Configuration::RECEIVER_N) % Configuration::PACKET_NUM;
		c = packet.seqnum >= this->rcvbase || packet.seqnum < (this->rcvbase + Configuration::RECEIVER_N) % Configuration::PACKET_NUM;
		if ((a&&b) || ((!a) && c))
		{
			cout << endl << "--------------------���շ������ڻ���ǰstart--------------------" << endl << endl;
			for (i = 0; i < Configuration::RECEIVER_N; i++)
			{
				if (this->RcvpacketWindow[i].rcv.seqnum != -1)
				{
					printf("������ţ�%d (%d)\t", this->RcvpacketWindow[i].rcv.seqnum, this->RcvpacketWindow[i].cache);
					pUtils->printPacket("�������ݣ�", this->RcvpacketWindow[i].rcv);
				}
			}
			cout << endl << "--------------------���շ������ڻ���ǰend--------------------" << endl << endl;
			i = 0;

			//���沢���
			this->RcvpacketWindow[(packet.seqnum - this->rcvbase + Configuration::PACKET_NUM) % Configuration::PACKET_NUM].cache = 1;
			this->RcvpacketWindow[(packet.seqnum - this->rcvbase + Configuration::PACKET_NUM) % Configuration::PACKET_NUM].rcv = packet;
			pUtils->printPacket("���շ��յ����ͷ��ı���", packet);
			if (this->RcvWinnum < Configuration::RECEIVER_N)
				this->RcvWinnum++;

			lastAckPkt.acknum = packet.seqnum;								//ȷ����ŵ����յ��ı������
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			pUtils->printPacket("�յ��������λ��[rcvbase, rcvbase + N - 1]:���շ�����ȷ�ϱ��Ĳ�������Ӧ", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);					//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�

			int preRcvBase = this->rcvbase;									//��¼�ƶ�����ǰ�Ļ����

			int flag = this->RcvpacketWindow[0].cache;
			while (this->RcvpacketWindow[i].cache && i < Configuration::RECEIVER_N)
			{	//�������ı��Ľ������ϲ�
				Message msg;
				memcpy(msg.data, this->RcvpacketWindow[i].rcv.payload, sizeof(this->RcvpacketWindow[i].rcv.payload));
				pns->delivertoAppLayer(RECEIVER, msg);
				this->rcvbase = (this->rcvbase + 1) % Configuration::PACKET_NUM;
				i++;
				this->RcvWinnum--;
			}

			int j = (this->rcvbase - preRcvBase + Configuration::PACKET_NUM) % Configuration::PACKET_NUM;
			i = 0;
			while (j < Configuration::RECEIVER_N)
				this->RcvpacketWindow[i++] = RcvpacketWindow[j++];	//�ƶ�����

			while (i < Configuration::RECEIVER_N)					//���ƶ���ճ����Ĵ�������
			{
				this->RcvpacketWindow[i].cache = 0;
				this->RcvpacketWindow[i++].rcv.seqnum = -1;
			}
			if (flag) {
				cout << endl << "rcvbase = seqnum�� �ƶ����ڣ�" << endl;
				cout << "--------------------���շ������ڻ�����start--------------------" << endl << endl;
				for (i = 0; i < Configuration::RECEIVER_N; i++)
				{
					if (this->RcvpacketWindow[i].rcv.seqnum != -1)
					{
						printf("������ţ�%d (%d)\t", this->RcvpacketWindow[i].rcv.seqnum, this->RcvpacketWindow[i].cache);
						pUtils->printPacket("�������ݣ�", this->RcvpacketWindow[i].rcv);
					}
				}
				cout << endl << "--------------------���շ������ڻ�����end--------------------" << endl << endl;
			}
			else {
				cout << endl << "rcvbase ��= seqnum�� ������Ӧ����״̬��" << endl;
				cout << "--------------------���շ������ڸ��º�start--------------------" << endl << endl;
				for (i = 0; i < Configuration::RECEIVER_N; i++)
				{
					if (this->RcvpacketWindow[i].rcv.seqnum != -1)
					{
						printf("������ţ�%d (%d)\t", this->RcvpacketWindow[i].rcv.seqnum, this->RcvpacketWindow[i].cache);
						pUtils->printPacket("�������ݣ�", this->RcvpacketWindow[i].rcv);
					}
				}
				cout << endl << "--------------------���շ������ڸ��º�end--------------------" << endl << endl;
			}

		}
		else {
			//���������[rcvbase-N, rcvbase-1]ʱ����ACK
			a = (this->rcvbase - Configuration::RECEIVER_N + Configuration::PACKET_NUM) % Configuration::PACKET_NUM < (this->rcvbase) % Configuration::PACKET_NUM;
			b = packet.seqnum >= (this->rcvbase - Configuration::RECEIVER_N + Configuration::PACKET_NUM) % Configuration::PACKET_NUM && packet.seqnum < (this->rcvbase) % Configuration::PACKET_NUM;
			c = packet.seqnum >= (this->rcvbase - Configuration::RECEIVER_N + Configuration::PACKET_NUM) % Configuration::PACKET_NUM || packet.seqnum < (this->rcvbase) % Configuration::PACKET_NUM;
			if ((a && b) || ((!a) && c))
			{
				lastAckPkt.acknum = packet.seqnum;								//ȷ����ŵ����յ��ı������
				lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
				pUtils->printPacket("�յ��������λ��[rcvbase - N, rcvbase - 1]:���շ���������ȷ�ϱ���", lastAckPkt);
				//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�
				pns->sendToNetworkLayer(SENDER, lastAckPkt);
			}
		}
	}
}