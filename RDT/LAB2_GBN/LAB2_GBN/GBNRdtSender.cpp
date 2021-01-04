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
	{	//sender处于等待确认状态
		return false;
	}

	if ((this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM < Configuration::WINDOWS_N)
	{	//窗口里存在未发送的分组
		this->packetWindow[this->winNum].acknum = -1;																//忽略该字段
		this->packetWindow[this->winNum].seqnum = this->nextseqnum;													//将确认号赋值给序号
		this->packetWindow[this->winNum].checksum = 0;
		memcpy(this->packetWindow[this->winNum].payload, message.data, sizeof(message.data));						//拷贝要发送的数据
		this->packetWindow[this->winNum].checksum = pUtils->calculateCheckSum(this->packetWindow[this->winNum]);	//计算校验和
		pUtils->printPacket("发送方发送报文", this->packetWindow[this->winNum]);									//打印数据报
		if (this->base == this->nextseqnum)
		{	//没有比nextseqnum更早的已发送但未被确认的分组，所以设置定时器
			pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWindow[this->winNum].seqnum);
		}
		pns->sendToNetworkLayer(RECEIVER, this->packetWindow[this->winNum]);										//递交网络层
		this->nextseqnum = (this->nextseqnum + 1) % Configuration::PACKET_NUM;										//更新nextseqnum
		this->winNum++;
		if (this->winNum == Configuration::WINDOWS_N)
		{	//窗口内不存在未发送的分组
			this->waitingState = true;
			return false;
		}
		this->waitingState = false;
		return true;
	}
	else
	{	//窗口里不存在未发送的分组
		this->waitingState = true;
		return false;
	}
}

void GBNRdtSender::receive(const Packet& ackPkt)
{
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	if (checkSum != ackPkt.checksum)
	{	//检查校验和是否正确
		cout << "发送方收到返回ACK，校验和出错！" << endl;
		return;
	}

	//判断是否收到不在窗口内的ack
	if (this->base <= this->nextseqnum) {
		if (ackPkt.acknum < this->base || ackPkt.acknum >= this->nextseqnum)
			return;
	}
	if (this->base > this->nextseqnum) {
		if (ackPkt.acknum < this->base && ackPkt.acknum >= this->nextseqnum)
			return;
	}
	
	cout << endl << "--------------------发送方：窗口滑动前--------------------" << endl;
	for (int i = 0; i < (this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM; i++)
	{
		cout << endl << "报文序号：" << this->packetWindow[i].seqnum << "	";
		pUtils->printPacket("报文内容：", this->packetWindow[i]);
	}
	cout << endl << "--------------------发送方：窗口滑动前--------------------" << endl << endl;
	
	int prebase = this->base;
	for (int i = 0; i <= (ackPkt.acknum - prebase + Configuration::PACKET_NUM) % Configuration::PACKET_NUM; i++)
	{	//移动窗口并关闭定时器
		pUtils->printPacket("发送方收到正确确认", ackPkt);
		pns->stopTimer(SENDER, this->packetWindow[i].seqnum);
		this->base = (this->base + 1) % Configuration::PACKET_NUM;
		this->winNum--;
	}
	int j = (this->base - prebase + Configuration::PACKET_NUM) % Configuration::PACKET_NUM, i = 0;
	while (j < (this->nextseqnum - prebase + Configuration::PACKET_NUM) % Configuration::PACKET_NUM)
		this->packetWindow[i++] = this->packetWindow[j++];

	cout << endl << "--------------------发送方：窗口滑动后--------------------" << endl;
	for (i = 0; i < (this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM; i++)
	{
		cout << endl << "报文序号：" << this->packetWindow[i].seqnum << "	";
		pUtils->printPacket("报文内容：", this->packetWindow[i]);
	}
	cout << endl << "--------------------发送方：窗口滑动后--------------------" << endl << endl;

	if (this->base == this->nextseqnum) {
		//没有已传输还未被确认的分组
	}
	else
	{
		for (i = 0; i < (this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM; i++)
		{	//为当前窗口中已发送还没有收到确认的分组开启定时器
			pns->stopTimer(SENDER, this->packetWindow[i].seqnum);
			pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWindow[i].seqnum);
		}
	}
	this->waitingState = false;
}

void GBNRdtSender::timeoutHandler(int seqNum)
{
	cout << endl << "超时重发：重传报文" << this->base << "以及其后的报文" << endl;
	for (int i = 0; i < this->winNum; i++)
	{
		pUtils->printPacket("重发", this->packetWindow[i]);
		pns->stopTimer(SENDER, this->packetWindow[i].seqnum);
		pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWindow[i].seqnum);
		pns->sendToNetworkLayer(RECEIVER, this->packetWindow[i]);
	}
}
