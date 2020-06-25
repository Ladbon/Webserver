//Server.cpp

#include <iostream>
#include "Server.h"
#include <fstream>

Server::Server()
{
	m_BuffLength = sizeof(m_RecvBuff);
}
Server::~Server()
{
	for (unsigned int i = 0; i < v_ClientList.size(); i++)
	{
		delete v_ClientList[i];
		v_ClientList[i] = nullptr;
		v_ClientList.erase(v_ClientList.begin() + i);
		--i;
	}
}
bool Server::init_Winsock()
{
	if (WSAStartup(MAKEWORD(2, 2), &m_net))
	{
		std::cout << "FAILED TO INIT WINSOCK" << std::endl;
		return false;
	}
	return true;
}
void Server::shutdown_Winsock()
{
	WSACleanup();
}
SOCKET Server::create_Socket()
{
	return socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}
void Server::socket_Destroy(SOCKET& _sock)
{
	closesocket(_sock);
}
void Server::setup_Local()
{
	m_localAddress.sin_family = AF_INET;
	m_localAddress.sin_addr.S_un.S_addr = INADDR_ANY;
	m_localAddress.sin_port = htons(8080);
	memset(m_localAddress.sin_zero, 0, sizeof(m_localAddress.sin_zero));
}
bool Server::Bind(SOCKET& _listensock)
{
	if (bind(
		_listensock,
		(const sockaddr*)&m_localAddress,
		sizeof(m_localAddress)) < 0)
	{
		std::cout << "COULD NOT BIND SOCKET" << std::endl;
		std::cout << "ERROR: " << WSAGetLastError() << std::endl;
		return false;
	}
	return true;
}
bool Server::setup_Listen(SOCKET& _listensock)
{
	if (listen(_listensock, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cout << "FAILED TO LISTEN ERROR: " << WSAGetLastError();
		return false;
	}
	return true;
}
bool Server::acceptUser(SOCKET _listensock)
{
	SOCKET userSock = INVALID_SOCKET;
	struct sockaddr_in Addr;
	int size = sizeof(Addr);

	//Tommi kod som Gregorius gav
	fd_set fd;
	fd.fd_count = 1;
	fd.fd_array[0] = _listensock;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 10;
	if (select(1, &fd, 0, 0, &tv) > 0)
	{
		std::cout << "ACCEPTING()" << std::endl;
		userSock = accept(_listensock, (struct sockaddr*)&Addr, &size);
		if (userSock == INVALID_SOCKET)
		{
			std::cout << "ACCEPT FAILED ERROR: " << WSAGetLastError() << std::endl;
			return false;
		}
		else
		{
			std::cout << "ADDING CLIENT" << std::endl;
			addUser(userSock, Addr);
		}
		return true;
	}
	return false;
}
void Server::addUser(SOCKET _Clientsock, struct sockaddr_in& _Addr)
{
	Client* client = new Client;
	client->s_sock = _Clientsock;
	client->s_address = _Addr;
	client->s_Timestamp = timeGetTime();
	client->s_Tick = 0;

	//KeepAlive
	client->KeepAlive = true;
	v_ClientList.push_back(client);
	std::cout << "NEW CONNECTION: ";

}
void Server::updateUserConnection()
{
	for (unsigned int i = 0; i < v_ClientList.size(); i++)
	{
		if (clienHasData(v_ClientList[i]) > 0)
		{
			std::cout << "HAS DATA" << std::endl;
			serverHandleData(v_ClientList[i]);
		}
		if (!clientCheckAlive(v_ClientList[i], i))
		{
			--i;
		}
	}
	std::cout << "ACTIVE CONNECTIONS: " << v_ClientList.size() << std::endl;
}
void Server::updateTimestamp(Client* _Client)
{
	_Client->s_Timestamp = timeGetTime();
}
int Server::clienHasData(Client* _Client)
{
	u_long len = 0;
	ioctlsocket(_Client->s_sock, FIONREAD, &len);
	return len;
}
bool Server::clientCheckAlive(Client* _Client,unsigned int& _Pos)
{
	int now = timeGetTime();
	if (_Client->KeepAlive)
	{
		if (now - _Client->s_Timestamp > 300000)
		{
			std::cout << "TIMEOUT: ";
			printClientAddress(&_Client->s_address);
			removeClient(_Pos);
			return false;
		}
	}
	else
		{
			std::cout << "Not a Keep-Alive connection, Closing: ";
			printClientAddress(&_Client->s_address);
			removeClient(_Pos);
			return false;
	}
	return true;
}
void Server::removeClient(unsigned int& _Pos)
{
	shutdownConnection(v_ClientList[_Pos]->s_sock);
	socket_Destroy(v_ClientList[_Pos]->s_sock);
	delete v_ClientList[_Pos];
	v_ClientList[_Pos] = nullptr;
	v_ClientList.erase(v_ClientList.begin() + _Pos);
}
bool Server::shutdownConnection(SOCKET& _ClientSock)
{
	if (shutdown(_ClientSock, SD_SEND == SOCKET_ERROR))
	{
		std::cout << "Error: failed to shutdown client" << WSAGetLastError() << std::endl;
		return false;
	}
	return true;
}
int Server::serverHandleData(Client* _Client)
{
	struct sockaddr_in remote;
	int size = sizeof(remote);

	int recvBytes = recvfrom(_Client->s_sock, m_RecvBuff, m_BuffLength - 1, 0, (struct sockaddr*)&remote, &size);
	if (recvBytes == SOCKET_ERROR)
	{
		std::cout << "Error recieving data: " << WSAGetLastError();
	}
	else if (recvBytes > 0)
	{
		m_RecvBuff[recvBytes] = '\0';

		updateTimestamp(_Client);

		std::string StrBuff = m_RecvBuff;
		std::string httpMethod = "";

		int start = StrBuff.find("GET ") + sizeof("GET");
		int length = StrBuff.find(" HTTP") - start;
		std::string subStr = StrBuff.substr(start, length);


		//Check Request
		if (subStr.find("/") != std::string::npos)
		{
			if (subStr == "/")
			{
				httpMethod = "index.html";
			}
			else if (subStr.find(".") != std::string::npos)
			{
				if (subStr.find(".", (subStr.find(".") + 1)) != std::string::npos)
					httpMethod = "404.html";
				else
					httpMethod = subStr;
			}
			else
			{
				httpMethod = subStr;
			}

		}

		if (StrBuff.find("Connection: ") != std::string::npos)
		{
			if (StrBuff.find("Keep-Alive", (StrBuff.find("Connection:") + 1)) != std::string::npos)
				_Client->KeepAlive = true;
			else if (StrBuff.find("keep-alive", (StrBuff.find("Connection:") + 1)) != std::string::npos)
				_Client->KeepAlive = true;
			else if (StrBuff.find("close", (StrBuff.find("Connection:") + 1)) != std::string::npos)
				_Client->KeepAlive = false;
			else if (StrBuff.find("Close", (StrBuff.find("Connection:") + 1)) != std::string::npos)
				_Client->KeepAlive = false;
		}
		serverSendData(_Client, httpMethod);

	}
	return 0;
}
bool Server::serverSendData(Client* _Client, const std::string _Type)
{
	m_HeaderStr = "";

	if (_Type.compare("404.html") != 0)
	{
		m_Headerinfo.s_HttpStatus = "200 OK";
	}

	std::cout << _Type << std::endl;

	std::ifstream page("page.txt");

	page.seekg(0, page.end);
	long size = page.tellg();
	page.seekg(0);

	char* buffer = new char[size];

	page.read(buffer, size);
	page.close();

	buffer[size] = '\0';

	int len = strlen(buffer);

	int total = 0;
	int bytesleft = len;
	int n;

	while (total < len)
	{
		n = send(_Client->s_sock, buffer + total, bytesleft, 0);
		if (n == -1){ break; }
		total += n;
		bytesleft -= n;
	}

	len = total;

	return n == -1 ? -1 : 0;
}
void Server::printClientAddress(struct sockaddr_in* _Addr)
{
	std::cout << (int)_Addr->sin_addr.S_un.S_un_b.s_b1 << ".";
	std::cout << (int)_Addr->sin_addr.S_un.S_un_b.s_b2 << ".";
	std::cout << (int)_Addr->sin_addr.S_un.S_un_b.s_b3 << ".";
	std::cout << (int)_Addr->sin_addr.S_un.S_un_b.s_b4 << std::endl;
}
void Server::setHeaderInfo()
{
	m_Headerinfo.s_Connection = "Keep-Alive";
	m_Headerinfo.s_ContentType = "text/html; image/jpeg";
	m_Headerinfo.s_CharSet = "utf - 8";
	m_Headerinfo.s_ServerName = "LadbonsServer";
}
void Server::updateHeader()
{
	setHeaderInfo();
	
	// create header string

	m_HeaderStr = "";
	m_HeaderStr += "HTTP/1.1 " + m_Headerinfo.s_HttpStatus + "\n";
	m_HeaderStr += "Content-Type: " + m_Headerinfo.s_ContentType + "; " + m_Headerinfo.s_CharSet + "\n";
	m_HeaderStr += "Content-length: " + std::to_string(m_Headerinfo.s_ContentLength) + "\n";
	m_HeaderStr += "Conncection: " + m_Headerinfo.s_Connection + "\n";
	m_HeaderStr += "Server: " + m_Headerinfo.s_ServerName + "\n";
	m_HeaderStr += "\n";
}