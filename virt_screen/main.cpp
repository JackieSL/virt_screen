#pragma comment(lib, "Ws2_32.lib")
#include "../usfl_lib/uslf_lib.hpp"


#include <WinSock2.h>
#include <WS2tcpip.h>


struct Connection {
	Connection() : handle(INVALID_SOCKET) 
	{
	}

	Connection(u32 h) : handle(h)
	{
	}

	u32 handle;
};

class ScreenServer
{
private:
	std::string address;
	std::string port;
	u32 handle;

	std::vector<Connection> connections;
	WSADATA wsaData;
public:
	ScreenServer(std::string address, std::string port) :	
		address(address),
		port(port) 
	{
		s32 iResult;
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			FATAL("WSAStartup failed: " << iResult);

		}
		handle = 0;


		struct addrinfo* result = NULL, * ptr = NULL, hints;

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		// Resolve the local address and port to be used by the server
		iResult = getaddrinfo(NULL, port.c_str(), &hints, &result);
		if (iResult != 0) {
			printf("getaddrinfo failed: %d\n", iResult);
			WSACleanup();
			FATAL("iResult: " << iResult);
		}

		handle = INVALID_SOCKET;
		handle = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

		if (handle == INVALID_SOCKET) {
			freeaddrinfo(result);
			WSACleanup();
			FATAL("Error at socket(): " << WSAGetLastError());
		}

		iResult = bind(handle, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			freeaddrinfo(result);
			closesocket(handle);
			WSACleanup();
			FATAL("bind failed with error: " << WSAGetLastError());
		}

		freeaddrinfo(result);
	}

	void Process()
	{
		char buffer[1024] = { 0 };

		for (auto connection : connections)
		{
			s32 iResult = 0;
			s32 iSendResult = 0;
			// Receive until the peer shuts down the connection
			do {

				iResult = recv(connection.handle, buffer, 1024, 0);
				if (iResult > 0) {
					printf("Bytes received: %d\n", iResult);

					// Echo the buffer back to the sender
					iSendResult = send(connection.handle, buffer, iResult, 0);
					if (iSendResult == SOCKET_ERROR) {
						closesocket(connection.handle);
						WSACleanup();
						FATAL("send failed with error: " << WSAGetLastError());
					}
					LOG("Bytes sent: " << iSendResult);
				}
				else if (iResult == 0)
				{
					FATAL("Connection closing...");
				}
				else {
					closesocket(connection.handle);
					WSACleanup();
					FATAL("recv failed with error: " << WSAGetLastError());
				}

			} while (iResult > 0);
		}

	}

	void Listen()
	{
		u32 client_socket = INVALID_SOCKET;
		u32 result = listen(this->handle, 1);

		if(result == SOCKET_ERROR)
		{
			closesocket(this->handle);
			WSACleanup();
			FATAL("listen failed with error: " <<  WSAGetLastError());
		}

		client_socket = accept(this->handle, NULL, NULL);
		this->connections.push_back(client_socket);
	}

	void Cleanup()
	{

	}
};

int main(int argc, char* argv[])
{
	if (argc > 1)
	{
		LOG("Arguments supplied by CMD:");
		for (int i = 0; i < argc; i++)
		{
			LOG(i + 1 << "/" << argc << ": " << argv[i]);
		}
		return 0;
	}

	LOG("No arguments supplied by CMD, initiating server @localhost:28500");

	ScreenServer server("localhost", "27500");
	server.Listen();
	while (true)
	{
		server.Process();
	}

	return -1;

}