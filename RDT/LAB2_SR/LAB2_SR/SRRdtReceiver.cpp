#include "stdafx.h"
#include "Global.h"
#include "SRRdtReceiver.h"


SRRdtReceiver::SRRdtReceiver()
{
	lastAckPkt.acknum = -1; //初始状态下，上次发送的确认包的确认序号为-1，使得当第一个接受的数据包出错时该确认报文的确认号为-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//忽略该字段
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
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(packet);
	int i = 0;

	//如果校验和正确，同时收到报文的序号有序
	if (checkSum == packet.checksum) {
		//窗口序号在[rcvbase, rcvbase+N-1]时发送ACK，缓存报文，并标记该窗口为已缓存
		int a, b, c;
		a = this->rcvbase <= (this->rcvbase + Configuration::RECEIVER_N) % Configuration::PACKET_NUM;
		b = packet.seqnum >= this->rcvbase && packet.seqnum < (this->rcvbase + Configuration::RECEIVER_N) % Configuration::PACKET_NUM;
		c = packet.seqnum >= this->rcvbase || packet.seqnum < (this->rcvbase + Configuration::RECEIVER_N) % Configuration::PACKET_NUM;
		if ((a&&b) || ((!a) && c))
		{
			cout << endl << "--------------------接收方：窗口滑动前start--------------------" << endl << endl;
			for (i = 0; i < Configuration::RECEIVER_N; i++)
			{
				if (this->RcvpacketWindow[i].rcv.seqnum != -1)
				{
					printf("报文序号：%d (%d)\t", this->RcvpacketWindow[i].rcv.seqnum, this->RcvpacketWindow[i].cache);
					pUtils->printPacket("报文内容：", this->RcvpacketWindow[i].rcv);
				}
			}
			cout << endl << "--------------------接收方：窗口滑动前end--------------------" << endl << endl;
			i = 0;

			//缓存并标记
			this->RcvpacketWindow[(packet.seqnum - this->rcvbase + Configuration::PACKET_NUM) % Configuration::PACKET_NUM].cache = 1;
			this->RcvpacketWindow[(packet.seqnum - this->rcvbase + Configuration::PACKET_NUM) % Configuration::PACKET_NUM].rcv = packet;
			pUtils->printPacket("接收方收到发送方的报文", packet);
			if (this->RcvWinnum < Configuration::RECEIVER_N)
				this->RcvWinnum++;

			lastAckPkt.acknum = packet.seqnum;								//确认序号等于收到的报文序号
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			pUtils->printPacket("收到报文序号位于[rcvbase, rcvbase + N - 1]:接收方发送确认报文并做出反应", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);					//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方

			int preRcvBase = this->rcvbase;									//记录移动窗口前的基序号

			int flag = this->RcvpacketWindow[0].cache;
			while (this->RcvpacketWindow[i].cache && i < Configuration::RECEIVER_N)
			{	//将连续的报文交付到上层
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
				this->RcvpacketWindow[i++] = RcvpacketWindow[j++];	//移动数组

			while (i < Configuration::RECEIVER_N)					//将移动后空出来的窗口置零
			{
				this->RcvpacketWindow[i].cache = 0;
				this->RcvpacketWindow[i++].rcv.seqnum = -1;
			}
			if (flag) {
				cout << endl << "rcvbase = seqnum， 移动窗口：" << endl;
				cout << "--------------------接收方：窗口滑动后start--------------------" << endl << endl;
				for (i = 0; i < Configuration::RECEIVER_N; i++)
				{
					if (this->RcvpacketWindow[i].rcv.seqnum != -1)
					{
						printf("报文序号：%d (%d)\t", this->RcvpacketWindow[i].rcv.seqnum, this->RcvpacketWindow[i].cache);
						pUtils->printPacket("报文内容：", this->RcvpacketWindow[i].rcv);
					}
				}
				cout << endl << "--------------------接收方：窗口滑动后end--------------------" << endl << endl;
			}
			else {
				cout << endl << "rcvbase ！= seqnum， 更新相应分组状态：" << endl;
				cout << "--------------------接收方：窗口更新后start--------------------" << endl << endl;
				for (i = 0; i < Configuration::RECEIVER_N; i++)
				{
					if (this->RcvpacketWindow[i].rcv.seqnum != -1)
					{
						printf("报文序号：%d (%d)\t", this->RcvpacketWindow[i].rcv.seqnum, this->RcvpacketWindow[i].cache);
						pUtils->printPacket("报文内容：", this->RcvpacketWindow[i].rcv);
					}
				}
				cout << endl << "--------------------接收方：窗口更新后end--------------------" << endl << endl;
			}

		}
		else {
			//窗口序号在[rcvbase-N, rcvbase-1]时发送ACK
			a = (this->rcvbase - Configuration::RECEIVER_N + Configuration::PACKET_NUM) % Configuration::PACKET_NUM < (this->rcvbase) % Configuration::PACKET_NUM;
			b = packet.seqnum >= (this->rcvbase - Configuration::RECEIVER_N + Configuration::PACKET_NUM) % Configuration::PACKET_NUM && packet.seqnum < (this->rcvbase) % Configuration::PACKET_NUM;
			c = packet.seqnum >= (this->rcvbase - Configuration::RECEIVER_N + Configuration::PACKET_NUM) % Configuration::PACKET_NUM || packet.seqnum < (this->rcvbase) % Configuration::PACKET_NUM;
			if ((a && b) || ((!a) && c))
			{
				lastAckPkt.acknum = packet.seqnum;								//确认序号等于收到的报文序号
				lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
				pUtils->printPacket("收到报文序号位于[rcvbase - N, rcvbase - 1]:接收方仅仅发送确认报文", lastAckPkt);
				//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方
				pns->sendToNetworkLayer(SENDER, lastAckPkt);
			}
		}
	}
}