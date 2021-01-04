#include"server.h"

int main() {
	WSADATA wsaData;
	//ȷ��socket�汾��Ϣ
	int nRc = WSAStartup(0x2020, &wsaData);

	if (nRc) {
		cout << "Winsock  startup failed with error!" << endl;
	}

	if (wsaData.wVersion != 0x0202) {
		cout << "Winsock version is not correct!" << endl;
	}

	printf("Winsock startup Ok!\n");
	//Э���壬�������ͣ�ָ���Ĵ���Э��
	//create socket
	SOCKET srvSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (srvSocket != INVALID_SOCKET)
		cout << "Socket create Ok!" << endl;
	//set port and ip
	struct sockaddr_in srvaddr;
	srvaddr.sin_family = AF_INET;
	srvaddr.sin_port = htons(Port);
	srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	//srvaddr.sin_addr.s_addr = inet_addr(IP_ADDR);
	//binding
	int rtn = bind(srvSocket, (LPSOCKADDR)&srvaddr, sizeof(srvaddr));
	if (rtn != SOCKET_ERROR)
		cout << "Socket bind Ok!" << endl;
	//listen
	rtn = listen(srvSocket, 5);
	if (rtn != SOCKET_ERROR)
		cout << "Socket listen Ok!" << endl;

	struct sockaddr_in cliaddr;
	int clilen = sizeof(cliaddr);
	while (1)
	{
		//accept
		SOCKET cliSocket = accept(srvSocket, (LPSOCKADDR)&cliaddr, &clilen);
		if (cliSocket != INVALID_SOCKET)
			cout << "Socket listen one client request!" << endl << endl;
		//reveive
		char recvdata[SIZE] = "";
		rtn = recv(cliSocket, recvdata, SIZE, 0);
		if (rtn) {
			getpeername(srvSocket, (LPSOCKADDR)&cliaddr, &clilen);
			cout << "������Դ��" << endl;
			char ip[16] = "";
			InetNtop(AF_INET, &cliaddr.sin_addr, ip, 100);
			//cout << "IP��ַ��" << inet_ntoa(cliaddr.sin_addr) << endl;
			cout << "IP��ַ��" << ip << endl;
			cout << "�˿ںţ�" << cliaddr.sin_port << endl;
			cout << recvdata;
		}
		HandleRequest(cliSocket, recvdata);
		closesocket(cliSocket);
	}
	closesocket(srvSocket);
	WSACleanup();
	getchar();
	return 0;
}

void HandleRequest(SOCKET cliSocket, char* recvdata)
{
	char filename[23] = { 0 };
	sscanf_s(recvdata, "GET /%s", filename, sizeof(filename));

	//�����ļ������mime����
	char mime[20];
	if (strstr(filename, ".html"))
		strcpy_s(mime, "text/html");
	else if (strstr(filename, ".txt"))
		strcpy_s(mime, "text/plain");
	else if (strstr(filename, ".gif"))
		strcpy_s(mime, "image/gif");
	else if (strstr(filename, ".jpg") || strstr(filename, ".jpeg"))
		strcpy_s(mime, "image/jpeg");
	else if (strstr(filename, ".au"))
		strcpy_s(mime, "audio/basic");
	else if (strstr(filename, ".mpg") || strstr(filename, ".mpeg"))
		strcpy_s(mime, "video/mpeg");
	else if (strstr(filename, ".avi"))
		strcpy_s(mime, "video/x-msvideo");
	else if (strstr(filename, ".gz"))
		strcpy_s(mime, "application/x-gzip");
	else {
		cout << "�ļ���ʽ�޷�����!" << endl << endl;
		Send404Info(cliSocket);
		return;
	}
	char buf[SIZE] = { 0 };
	ifstream is;
	is.open(filename, ios::binary);
	if (is.is_open()) {
		sprintf_s(buf, "HTTP/1.1 200 0K\r\nContent-Type: %s\r\n\r\n", mime);
		send(cliSocket, buf, strlen(buf), 0);
		do {
			is.read(buf, SIZE);
			send(cliSocket, buf, is.gcount(), 0);
		} while (is.gcount() == SIZE);
		cout << "������ɹ�!" << endl << endl;
	}
	else {
		cout << "�ļ�������!" << endl << endl;
		Send404Info(cliSocket);
	}
	is.close();
}

void Send404Info(SOCKET cliSocket) {
	ifstream is;
	char buf[SIZE] = { 0 };
	char NotFoundData[30] = "HTTP/1.1 404 Not Found\r\n\r\n";
	send(cliSocket, NotFoundData, strlen(NotFoundData), 0);
	is.open("notfound.html", ios::binary);
	do {
		is.read(buf, SIZE);
		send(cliSocket, buf, is.gcount(), 0);
	} while (is.gcount() == SIZE);
}
