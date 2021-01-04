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
	{	//发送方处于等待确认状态
		return false;
	}

	if ((this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM < Configuration::SENDER_N)
	{
		this->packetWindow[this->winNum].acknum = -1;																//忽略该字段
		this->packetWindow[this->winNum].seqnum = this->nextseqnum;													//将确认号赋值给序号
		this->packetWindow[this->winNum].checksum = 0;
		memcpy(this->packetWindow[this->winNum].payload, message.data, sizeof(message.data));						//拷贝要发送的数据
		this->packetWindow[this->winNum].checksum = pUtils->calculateCheckSum(this->packetWindow[this->winNum]);	//计算检验和
		pUtils->printPacket("发送方发送报文", this->packetWindow[this->winNum]);									//打印数据报
		if (this->base == this->nextseqnum)
			pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWindow[this->winNum].seqnum);				//启动发送方定时器
		pns->sendToNetworkLayer(RECEIVER, this->packetWindow[this->winNum]);										//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
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
	{	//窗口已满
		this->waitingState = true;
		return false;
	}
}

void TCPRdtSender::receive(const Packet& ackPkt) {
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	if (checkSum != ackPkt.checksum)
	{
		cout << "发送方收到确认报文，校验和出错！" << endl;
		return;
	}

	//收到冗余ACK，快速重传
	dupAck[this->count % 3] = ackPkt.acknum;
	if (dupAck[0] == dupAck[1] && dupAck[1] == dupAck[2])
	{
		cout << endl << "------快速重传------" << endl << endl;
		pUtils->printPacket("发送方收到3个冗余ACK", ackPkt);
		pUtils->printPacket("快速重传：重传最早未确认的报文", this->packetWindow[0]);
		pns->stopTimer(SENDER, this->packetWindow[0].seqnum);									//首先关闭定时器
		pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWindow[0].seqnum);			//重新启动发送方定时器
		pns->sendToNetworkLayer(RECEIVER, this->packetWindow[0]);
		cout << endl << "------快速重传------" << endl << endl;
	}
	this->count++;
	//判断收到的ACK是否在窗口内
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
	cout << endl << "--------------------发送方：窗口滑动前start--------------------" << endl << endl;
	for (i = 0; i < (this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM; i++)
	{
		printf("报文序号：%d\t", this->packetWindow[i].seqnum);
		pUtils->printPacket("报文内容：", this->packetWindow[i]);
	}
	cout << endl << "--------------------发送方：窗口滑动前end--------------------" << endl << endl;

	//如果校验和正确，移动窗口，关闭定时器
	int preBase = this->base;				//移动前窗口基序号
	for (i = 0; i <= (ackPkt.acknum - preBase + Configuration::PACKET_NUM) % Configuration::PACKET_NUM; i++)
	{
		pUtils->printPacket("发送方正确收到确认", ackPkt);
		pns->stopTimer(SENDER, this->packetWindow[i].seqnum);
		this->base = (this->base + 1) % Configuration::PACKET_NUM;
		this->winNum--;	
	}
	//移动窗口
	int j = (this->base - preBase + Configuration::PACKET_NUM) % Configuration::PACKET_NUM;				//未确认分组序号
	i = 0;
	while (j < (this->nextseqnum - preBase + Configuration::PACKET_NUM) % Configuration::PACKET_NUM)
		this->packetWindow[i++] = packetWindow[j++];	//移动数组
	cout << endl << "--------------------发送方：窗口滑动后start--------------------" << endl << endl;
	for (i = 0; i < (this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM; i++)
	{
		printf("报文序号：%d\t", this->packetWindow[i].seqnum);
		pUtils->printPacket("报文内容：", this->packetWindow[i]);
	}
	cout << endl << "--------------------发送方：窗口滑动后end--------------------" << endl << endl;

	if (this->base == this->nextseqnum) {
	}
	else
	{
		//为当前窗口中还没有收到确认的报文开启定时器
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
		cout << endl << "------超时重传------" << endl << endl;
		pUtils->printPacket("超时重传：重传最早未确认的报文", this->packetWindow[0]);
		pns->stopTimer(SENDER, this->packetWindow[0].seqnum);									//首先关闭定时器
		pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWindow[0].seqnum);			//重新启动发送方定时器
		pns->sendToNetworkLayer(RECEIVER, this->packetWindow[0]);								//重新发送数据包
		cout << endl << "------超时重传------" << endl << endl;
	}
}
