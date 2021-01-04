#include "stdafx.h"
#include "Global.h"
#include "SRRdtSender.h"

SRRdtSender::SRRdtSender() :nextseqnum(0), base(0), winNum(0), waitingState(false)
{
	for (int i = 0; i < Configuration::SENDER_N; i++)
		packetWindow[i].Ack = 0;	//是否已接受确认ACK初始化
}

SRRdtSender::~SRRdtSender()
{
}

bool SRRdtSender::getWaitingState() {
	return this->waitingState;
}


bool SRRdtSender::send(const Message& message) {
	if (this->waitingState)
	{	//发送方处于等待确认状态
		return false;
	}

	if ((this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM < Configuration::SENDER_N)
	{
		this->packetWindow[this->winNum].pkt.acknum = -1;																	//忽略该字段
		this->packetWindow[this->winNum].pkt.seqnum = this->nextseqnum;														//将确认号赋值给序号
		this->packetWindow[this->winNum].pkt.checksum = 0;
		this->packetWindow[this->winNum].Ack = 0;
		memcpy(this->packetWindow[this->winNum].pkt.payload, message.data, sizeof(message.data));							//拷贝要发送的数据
		this->packetWindow[this->winNum].pkt.checksum = pUtils->calculateCheckSum(this->packetWindow[this->winNum].pkt);	//计算检验和
		pUtils->printPacket("发送方发送报文", this->packetWindow[this->winNum].pkt);										//打印数据报
		if (this->base == this->nextseqnum)
			pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWindow[this->winNum].pkt.seqnum);					//启动发送方定时器
		pns->sendToNetworkLayer(RECEIVER, this->packetWindow[this->winNum].pkt);											//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
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

void SRRdtSender::receive(const Packet& ackPkt) {
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	if (checkSum != ackPkt.checksum)
	{	//校验和出错
		cout << "发送方收到确认报文，校验和出错！" << endl;
		return;
	}
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
		printf("报文序号：%d (%d)\t", this->packetWindow[i].pkt.seqnum, this->packetWindow[i].Ack);
		pUtils->printPacket("报文内容：", this->packetWindow[i].pkt);
	}
	cout << endl << "--------------------发送方：窗口滑动前end--------------------" << endl << endl;
	i = 0;

	int preBase = this->base;																							//移动前窗口基序号
	this->packetWindow[(ackPkt.acknum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM].Ack = 1;	//标记已接受
	pUtils->printPacket("发送方收到接收方的确认", ackPkt);
	pns->stopTimer(SENDER, this->packetWindow[(ackPkt.acknum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM].pkt.seqnum);

	//如果收到的确认等于当前的基序号，移动窗口
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
			this->packetWindow[i++] = packetWindow[j++];	//移动数组
		while (i < Configuration::SENDER_N)					
			this->packetWindow[i++].Ack = 0;				//将移动后空出来的窗口置零

		cout << endl << "sendbase = ACK， 移动窗口：" << endl;
		cout << "--------------------发送方：窗口滑动后start--------------------" << endl << endl;
		for (i = 0; i < (this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM; i++)
		{
			printf("报文序号：%d (%d)\t", this->packetWindow[i].pkt.seqnum, this->packetWindow[i].Ack);
			pUtils->printPacket("报文内容：", this->packetWindow[i].pkt);
		}
		cout << endl << "--------------------发送方：窗口滑动后end--------------------" << endl << endl;
	}
	else
	{
		cout << endl << "sendbase ！= ACK， 更新相应分组状态：" << endl;
		cout << "--------------------发送方：窗口内容更新后start--------------------" << endl << endl;
		for (i = 0; i < (this->nextseqnum - this->base + Configuration::PACKET_NUM) % Configuration::PACKET_NUM; i++)
		{
			printf("报文序号：%d (%d)\t", this->packetWindow[i].pkt.seqnum, this->packetWindow[i].Ack);
			pUtils->printPacket("报文内容：", this->packetWindow[i].pkt);
		}
		cout << endl << "--------------------发送方：窗口内容更新后end--------------------" << endl << endl;
	}

	if (this->base == this->nextseqnum) {
		this->waitingState = false;
	}
	else
	{
		i = 0;
		//为当前窗口中还没有收到确认的报文开启定时器
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
			cout << endl << "------超时重传------" << endl << endl;
			pUtils->printPacket("超时重传", this->packetWindow[i].pkt);
			pns->stopTimer(SENDER, this->packetWindow[i].pkt.seqnum);									//首先关闭定时器
			pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWindow[i].pkt.seqnum);			//重新启动发送方定时器
			pns->sendToNetworkLayer(RECEIVER, this->packetWindow[i].pkt);								//重新发送数据包
			cout << endl << "------超时重传------" << endl << endl;
		}
	}
}
