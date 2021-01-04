#include "GBNRdtReceiver.h"
#include "stdafx.h"
#include "Global.h"

GBNRdtReceiver::GBNRdtReceiver() : expectSequenceNumberRcvd(0)
{
	lastAckPkt.acknum = -1;	//��ʼ״̬�£��ϴη��͵�ȷ�ϰ���ȷ�����Ϊ-1��ʹ�õ���һ�����ܵ����ݰ�����ʱ��ȷ�ϱ��ĵ�ȷ�Ϻ�Ϊ-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
		lastAckPkt.payload[i] = '.';
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}

GBNRdtReceiver::~GBNRdtReceiver()
{
}

void GBNRdtReceiver::receive(const Packet& packet) {
	int checkSum = pUtils->calculateCheckSum(packet);
	if (checkSum == packet.checksum && this->expectSequenceNumberRcvd == packet.seqnum)
	{	//У�����ȷ�����յ��ı�������
		pUtils->printPacket("���շ���ȷ�յ����ͷ��ı���", packet);
		//ȡ��Message�����ϵݽ���Ӧ�ò�
		Message msg;
		memcpy(msg.data, packet.payload, sizeof(packet.payload));
		pns->delivertoAppLayer(RECEIVER, msg);

		lastAckPkt.acknum = packet.seqnum;
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pUtils->printPacket("���շ�����ȷ��ACK", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);

		this->expectSequenceNumberRcvd = (this->expectSequenceNumberRcvd + 1) % Configuration::PACKET_NUM;
	}
	else
	{
		if (checkSum != packet.checksum)
			pUtils->printPacket("���շ�û����ȷ���շ��ͷ��ı��ģ�����У�����", packet);
		else
			pUtils->printPacket("���շ�û����ȷ���ܷ��ͷ��ı��ģ�������Ų���", packet);
		
		pUtils->printPacket("���շ����·����ϴε�ȷ�ϱ���", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);
	}
}
