#include "GBNRdtReceiver.h"
#include "stdafx.h"
#include "Global.h"

GBNRdtReceiver::GBNRdtReceiver() : expectSequenceNumberRcvd(0)
{
	lastAckPkt.acknum = -1;	//初始状态下，上次发送的确认包的确认序号为-1，使得当第一个接受的数据包出错时该确认报文的确认号为-1
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
	{	//校验和正确并且收到的报文有序
		pUtils->printPacket("接收方正确收到发送方的报文", packet);
		//取出Message，向上递交给应用层
		Message msg;
		memcpy(msg.data, packet.payload, sizeof(packet.payload));
		pns->delivertoAppLayer(RECEIVER, msg);

		lastAckPkt.acknum = packet.seqnum;
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pUtils->printPacket("接收方发送确认ACK", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);

		this->expectSequenceNumberRcvd = (this->expectSequenceNumberRcvd + 1) % Configuration::PACKET_NUM;
	}
	else
	{
		if (checkSum != packet.checksum)
			pUtils->printPacket("接收方没有正确接收发送方的报文，数据校验错误", packet);
		else
			pUtils->printPacket("接收方没有正确接受发送方的报文，报文序号不对", packet);
		
		pUtils->printPacket("接收方重新发送上次的确认报文", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);
	}
}
