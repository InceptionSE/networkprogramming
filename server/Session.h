#include <WinSock2.h>
#include<stdio.h>

/**
*	@struct Session: a session between a client and server
*	@datafield connSock: Socket identifier on the server
*   @datafield clientIP: Client Address
*	@datafield username: username logged in at client
*   @datafield clientPort: client Port
*   @datafield status: client login status
**/
struct Session {
	SOCKET connSock;
	char clientIP[22];
	char username[20];
	int clientPort;
	int status = -1;//-1: unused   0: in used   1: chatting
};