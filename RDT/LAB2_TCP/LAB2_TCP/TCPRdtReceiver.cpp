#include "TCPRdtReceiver.h"
#include "Global.h"
#include "stdafx.h"

TCPRdtReceiver::TCPRdtReceiver() :expectSequenceNumberRcvd(0)
{
	lastAckPkt.acknum = -1; //初始状态下，上次发送的确认包的确认序号为-1，使得当第一个接受的数据包出错时该确认报文的确认号为-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//忽略该字段
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
		lastAckPkt.payload[i] = '.';
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}

TCPRdtReceiver::~TCPRdtReceiver()
{
}

void TCPRdtReceiver::receive(const Packet& packet) {
	//检查校验和
	int checkSum = pUtils->calculateCheckSum(packet);

	if (checkSum == packet.checksum && this->expectSequenceNumberRcvd == packet.seqnum)
	{	//校验和正确并且收到报文的序列号与期待的相同
		pUtils->printPacket("接收方正确收到发送方的报文", packet);

		//取出Message，向上递交给应用层
		Message msg;
		memcpy(msg.data, packet.payload, sizeof(packet.payload));
		pns->delivertoAppLayer(RECEIVER, msg);

		lastAckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pUtils->printPacket("接收方发送确认报文", lastAckPkt);

		//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方
		pns->sendToNetworkLayer(SENDER, lastAckPkt);

		this->expectSequenceNumberRcvd = (this->expectSequenceNumberRcvd + 1) % Configuration::PACKET_NUM;
	}
	else
	{
		if (checkSum != packet.checksum) {
			pUtils->printPacket("接收方没有正确收到发送方的报文,数据校验错误", packet);
		}
		else {
			pUtils->printPacket("接收方没有正确收到发送方的报文,报文序号不对", packet);
		}
		pUtils->printPacket("接收方重新发送上次的确认报文", lastAckPkt);

		//调用模拟网络环境的sendToNetworkLayer，通过网络层发送上次的确认报文
		pns->sendToNetworkLayer(SENDER, lastAckPkt);
	}
}