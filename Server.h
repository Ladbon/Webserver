//Server.h

#pragma once

#include <winsock2.h>
#include <Windows.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

class Server
{
private:

	struct Client
	{
		SOCKET s_sock;
		sockaddr_in s_address;
		unsigned int s_Timestamp;
		unsigned int s_Tick;
		bool KeepAlive;
	};
	struct Header
	{
		//Header info
		std::string s_HttpStatus;
		std::string s_ContentType;
		std::string s_CharSet;
		int s_ContentLength;
		std::string s_Connection;
		std::string s_ServerName;
	};

	WSAData m_net;
	std::vector<Client*> v_ClientList;
	struct sockaddr_in m_localAddress;
	char m_RecvBuff[8192];
	int m_BuffLength;
	Header m_Headerinfo;
	std::string m_HeaderStr;
	char *m_OutBuffer;
public:

	Server();
	~Server();
	bool init_Winsock();
	void shutdown_Winsock();
	
	SOCKET create_Socket();
	void socket_Destroy(SOCKET& _sock);
	
	void setup_Local();

	bool Bind(SOCKET& _listensock);
	bool setup_Listen(SOCKET& _listensock);

	bool acceptUser(SOCKET _listensock);
	void addUser(SOCKET _Clientsock, struct sockaddr_in& _Addr);

	void updateUserConnection();
	void updateTimestamp(Client* _Client);
	bool clientCheckAlive(Client* _Client,unsigned int& _Pos);

	int clienHasData(Client* _Client);
	int serverHandleData(Client* _Client);
	bool serverSendData(Client* _Client, const std::string _Type);

	void printClientAddress(struct sockaddr_in* _Addr);
	bool shutdownConnection(SOCKET& _ClientSock);
	void removeClient(unsigned int& _Pos);

	void setHeaderInfo();
	void updateHeader();
};