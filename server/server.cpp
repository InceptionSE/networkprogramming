// TCP_Server.cpp : Defines the entry point for the console application.
//
#include "stdio.h"
#include "conio.h"
#include "string.h"
#include "ws2tcpip.h"
#include "winsock2.h"
#include "process.h"
#include "Session.h"
#include "PrivateChat.h"
#include "RoomChat.h"
#include<vector>
#include<map>
#pragma comment(lib, "Ws2_32.lib")
#define SERVER_PORT "5500"
#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 2048
using namespace std;
/**
*	@function chartoInt: convert a string to number (int)
*	@param s: A pointer to a input string
*	@return: a number
**/

CRITICAL_SECTION critical;
vector<Session> listOnline;
PrivateChat listPrivateChat[100];
RoomChat listRoomChat[100];
char str[40] = "~!@#$%^&*()_-+={[}];:',<.>/?";

int chartoInt(char *s);
void inttoChar(char des[], int number);
void split(char* str, char** header, char** data);
int handleLogin(char* data, Session &session);
int handleRegister(char* data, Session session);
int handleLogout(Session &session);
int handleMessage(char* str, Session &session);
int sendListUserOnline(SOCKET connect);
unsigned __stdcall echoThread(void *param);


int main(int argc, char* argv[])
{	
	InitializeCriticalSection(&critical);
	//Step 1: Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("Winsock 2.2 is not supported\n");
		return 0;
	}

	//Step 2: Construct socket	
	SOCKET listenSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//Step 3: Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(chartoInt(SERVER_PORT));
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);
	if (bind(listenSock, (sockaddr *)&serverAddr, sizeof(serverAddr)))
	{
		printf("Error %d: Cannot associate a local address with server socket.", WSAGetLastError());
		return 0;
	}

	//Step 4: Listen request from client
	if (listen(listenSock, 10)) {
		printf("Error %d: Cannot place server socket in state LISTEN.", WSAGetLastError());
		return 0;
	}
	printf("Server started!\n");
	//Step 5: Communicate with client
	SOCKET connSock;
	//Session session;
	sockaddr_in clientAddr;
	char clientIP[INET_ADDRSTRLEN];
	int clientAddrLen = sizeof(clientAddr), clientPort;
	while (1) {
		connSock = accept(listenSock, (sockaddr *)& clientAddr, &clientAddrLen);
		if (connSock == SOCKET_ERROR)
			printf("Error %d: Cannot permit incoming connection.\n", WSAGetLastError());
		else {
			inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
			clientPort = ntohs(clientAddr.sin_port);
			printf("Accept incoming connection from %s:%d\n", clientIP, clientPort);
			//initialize
			Session session;
			memset(&session, 0, sizeof(Session));
			session.connSock = connSock;
			strcpy(session.clientIP, clientIP);
			session.clientPort = clientPort;
			session.status = -1;
			strcpy(session.username, "");
			_beginthreadex(0, 0, echoThread, (void *)&session, 0, 0); //start thread
		}
	}
	closesocket(listenSock);
	DeleteCriticalSection(&critical);
	WSACleanup();
	return 0;
}

/* echoThread - Thread to receive the message from client and echo*/
unsigned __stdcall echoThread(void *param) {
	char buff[BUFF_SIZE], buff_send[BUFF_SIZE];
	string buff_recv = "";
	int ret;

	//vector<char*> requestList;
	Session session = *(Session *)param;
	//printf("*%s*\n", session->username);
	SOCKET connectedSocket = (SOCKET)(session.connSock);
	while (1) {
		ret = recv(connectedSocket, buff, BUFF_SIZE, 0);
		//printf("*%d**\n", listOnline.size(), buff);
		if (ret == SOCKET_ERROR) {
			printf("Error %d: Cannot receive data.\n", WSAGetLastError());
			printf("Clien [%s:%d] disconnect\n", session.clientIP, session.clientPort);
			session.status = -1;
			strcpy_s(session.username, 1024, "");
			break;
		}
		else if (ret > 0) {
			buff[ret] = 0;
			printf("Receive from client[%s:%d] %s\n", session.clientIP, session.clientPort, buff);
			
			
				//Handling messages
				char message[30];
				int start = 0;
				for (int i = start; i < strlen(buff); i++) {
					if (buff[i] == '\r'&&buff[i + 1] == '\n') {
						strncpy(message, buff + start, i - start);
						message[i - start] = '\0';
						string message_str = message;
						start = i + 2;
						buff_recv += message_str;
						char s[BUFF_SIZE];
						strcpy_s(s, BUFF_SIZE, buff_recv.c_str());
						//printf("Message: %s*\n", s);
						//printf("---%s---\n", session.username);
						int res = handleMessage(s, session);
						buff_recv = "";
						printf("--->%d\n", res);
						if (res > 0 && res != 50 && res != 40 && res != 44 && res!=30) {
							itoa(res, buff_send, 10);
							//Echo to client
							ret = send(connectedSocket, buff_send, strlen(buff_send), 0);
							if (ret == SOCKET_ERROR) {
								printf("Error %d: Cannot send data.\n", WSAGetLastError());
								break;
							}
						}
					}  
				}
				if (start < ret) {
					buff_recv += buff;
				}
		}
		else {//ret == 0 -> client disconnect
			printf("Clien [%s:%d] disconnect\n", session.clientIP, session.clientPort);
			session.status = -1;
			strcpy_s(session.username, 1024, "");
			break;
		}
	}
	shutdown(connectedSocket, SD_SEND);
	closesocket(connectedSocket);
	return 0;
}

int chartoInt(char *s)
{
	int res = 0;
	for (int i = 0; i < strlen(s); i++)
	{
		res = res * 10 + (s[i] - '0');
	}
	return res;
}

/**
*	@function inttoChar: convert a number to a string
*	@param number: input number
*	@return: a string after convert
**/
void inttoChar(char res[], int number) {
	if (number == 0) {
		strcpy(res, "0");
		return;
	}
	int i = 0;
	while (number > 0) {
		res[i] = number % 10 + '0';
		number /= 10; \
			i++;
	}
	res[i] = 0;
	for (i = 0; i < strlen(res) / 2; i++) {
		int t = res[i];
		res[i] = res[strlen(res) - i - 1];
		res[strlen(res) - i - 1] = t;
	}
}


/**
*	@function split: Separate the header and data portion of a message with a space
*	@param header: message part before space
*   @param data: message part after space
**/
void split(char* str, char** header, char** data) {

	*header = strtok(str, " ");
	*data = str + strlen(*header) + 1;

}

/**
*	@handleLogin: return the code when user login
*	@param data: username entered by user
*   @param session: a session between a client and server
*	@return: a number
**/
int handleLogin(char* data, Session & session) {
	//open file
	FILE * fp = NULL;
	char arr[128];
	char* username;
	char* password;
	char* fusername;
	char* fpassword;
	fp = fopen("D:\\account.txt", "r");
	split(data, &username, &password);
	while (fgets(arr, 128, fp) != NULL)
	{
		split(arr, &fusername, &fpassword);

		
		for (int i = 0; i <= strlen(fpassword); i++) {
			if (fpassword[i] == ' ' || fpassword[i] == '\n') fpassword[i] = '\0';
		}
		if (!strcmp(username, fusername)) {
			if ((session).status > -1) return 14;//da dang nhap tren client do
			if (!strcmp(password, fpassword)) {

				strcpy((session).username,username);
				(session).status = 0;
				///// Dang nhap thanh cong
				EnterCriticalSection(&critical);
				//Da dang nhap tren client khac
				for (int i = 0; i < listOnline.size(); i++) {
					if (!strcmp(username, listOnline[i].username)) return 12;
				}
				listOnline.push_back(session);
				
				LeaveCriticalSection(&critical);
				////////
				return 10;
			}
			else return 13;
		}

	}
	fclose(fp);
	return 11;
}

int handleRegister(char* data, Session session) {
	//open file
	FILE * fp = NULL;
	FILE * fw = NULL;
	char arr[128];
	char* username;
	char* password;
	char* fusername;
	char* fpassword;
	fp = fopen("D:\\account.txt", "r");
	if (fp == NULL) {
		fp = fopen("D:\\account.txt", "w");
	}
	else {
		split(data, &username, &password);
		while (fgets(arr, 128, fp) != NULL)
		{
			split(arr, &fusername, &fpassword);
			if (!strcmp(username, fusername)) {
				return 21;
			}

		}
		for (int i = 0; i < strlen(username); i++)
			for (int j = 0; j<strlen(str); j++)
				if (str[j] == username[i]) return 22;
		fclose(fp);
		fw = fopen("D:\\account.txt", "a+");
		fprintf(fw, "%s %s\n", username, password);
	}

	fclose(fw);
	return 20;
}

/**
*	@handleLogout: return the code when user log out
*   @param session: a session between a client and server
*	@return: a number
**/
int handleLogout(Session & session) {
	if ((session).status > -1) {
		(session).status = -1;
		//EnterCriticalSection(&critical);
		//Da dang nhap tren client khac
		for (int i = 0; i < listOnline.size(); i++) {
			if (!strcmp((session).username, listOnline[i].username))
			{
				listOnline.erase(listOnline.begin() + i);
				break;
			}
		}
		//LeaveCriticalSection(&critical);
		strcpy((session).username,"");
		return 15;
	}
	else return 16;
}

int handleRequestChat(char* partnerName, Session session) {
	//printf("*%s-%s*\n", session.username, partnerName);
	int d = 0;
	int index1 = -1;
	int index2 = -1;
	for (int i = 0; i < listOnline.size(); i++) {
		if (!strcmp((session).username, listOnline[i].username))
		{
			index1 = i;
			break;
		}
	}
	if (index1 == -1)
		return 16;//user not online
	for (int i = 0; i < listOnline.size(); i++) {
		if (i != index1 && !strcmp(partnerName, listOnline[i].username))
		{	
			if (listOnline[i].status == 1)
				return 66;
			index2 = i;
			char requestChatBuf[BUFF_SIZE];
			strcpy(requestChatBuf, "REQUEST_CHAT ");
			strcat(requestChatBuf, (session).username);
			//strcat(requestChatBuf, "\r\n");
			int ret = send(listOnline[i].connSock, requestChatBuf, strlen(requestChatBuf), 0);
			if (ret == SOCKET_ERROR) {
				printf("Send request chat fail with code: %d \n", WSAGetLastError());
				return 54;
			}
			int j = 0;
			//EnterCriticalSection(&critical);
			for (j = 0; j < 100; j++) 
				if (listPrivateChat[j].status == -1){
					listPrivateChat[j].status = 0;
					listPrivateChat[j].user1Idx = index1;
					listPrivateChat[j].user2Idx = index2;
					break;
				}
			//LeaveCriticalSection(&critical);
			if (j == 100) {
				printf("Full Room!\n");
				return 99;
			}
			break;
			return 50;
		}
	}
	if (index2 == -1) {//partner not online
		printf("not online");
		return 54;
	}
	return 50;
}

int handleResponseChat(char* data, Session session) {
	char* partnerName;
	char* isAcceptStr;
	split(data, &partnerName, &isAcceptStr);
	int isAccept = chartoInt(isAcceptStr);

	int i = 0;//room index
	int n = listOnline.size();//amount of onl users
	int ui1 = -1;
	int ui2 = -1;
	for (i = 0; i < 100; i++)
		if (listPrivateChat[i].status == 0) {//waiting for response
			if (listPrivateChat[i].user1Idx >= 0 && listPrivateChat[i].user2Idx >= 0
				&& listPrivateChat[i].user1Idx < n && listPrivateChat[i].user1Idx < n) {
				int idx1 = listPrivateChat[i].user1Idx;
				int idx2 = listPrivateChat[i].user2Idx;
				if (!strcmp(listOnline[idx1].username, partnerName)
					&& !strcmp(listOnline[idx2].username, session.username)) {
					ui1 = idx1;
					ui2 = idx2;
					break;
				}
			}
		}
	if (ui1 == -1 || ui2 == -1)
		return 99;
	if (isAccept == 1) {//accept
		//EnterCriticalSection(&critical);
		listPrivateChat[i].status = 1;
		listOnline[ui1].status = 1;
		listOnline[ui2].status = 1;
		//LeaveCriticalSection(&critical);

		char responseChat[BUFF_SIZE] = "RESPONSE_CHAT ";
		char roomIdxStr[15];
		inttoChar(roomIdxStr, i + 1);
		strcat(responseChat, roomIdxStr);
	
		printf("---%s---\n", responseChat);
		int ret = send(listOnline[ui1].connSock, responseChat, strlen(responseChat), 0);
		if (ret == SOCKET_ERROR)
			return 99;
		ret = send(listOnline[ui2].connSock, responseChat, strlen(responseChat), 0);
		if (ret == SOCKET_ERROR)
			return 99;
		return 50;
		}
		else {//not accept
			//EnterCriticalSection(&critical);
			listPrivateChat[i].status = -1;
			listPrivateChat[i].user1Idx = -1;
			listPrivateChat[i].user2Idx = -1;
			//LeaveCriticalSection(&critical);

			char responseChat[BUFF_SIZE] = "RESPONSE_CHAT 0";
			strcat(responseChat, "\r\n");
			//printf("%s****\n", responseChat);
			int ret = send(listOnline[ui1].connSock, responseChat, strlen(responseChat), 0);
			if (ret == SOCKET_ERROR) {
				printf("error");
				return 99;
			}
			session.status = 0;
			return 53;
		}
}

int handleSendChat(char* data, Session session) {
	char* roomIdStr;
	char* message;
	split(data, &roomIdStr, &message);
	int roomId = chartoInt(roomIdStr) - 1;
	if (roomId < 0 && roomId >= 100)
		return 99;
	int n = listOnline.size();
	int ui1 = listPrivateChat[roomId].user1Idx;
	int ui2 = listPrivateChat[roomId].user2Idx;
	if (listPrivateChat[roomId].status == 1 &&  ui1>= 0 && ui2 >= 0 
		&& ui1 < n && ui2 < n) {

		int recvIdx;
		if (!strcmp(session.username, listOnline[ui1].username))
			recvIdx = ui2;
		else
			recvIdx = ui1;
		char sendChat[BUFF_SIZE] = "SEND ";
		
		strcat(sendChat, listOnline[recvIdx].username);
		strcat(sendChat, " ");
		strcat(sendChat, message);

		int ret = send(listOnline[recvIdx].connSock, sendChat, strlen(sendChat), 0);
		if (ret == SOCKET_ERROR)
			return 99;
		return 40;
	}
	else {
		return 42;
	}
}

int handleEndChat(char* data, Session session) {
	int roomId = chartoInt(data) - 1;
	if (roomId < 0 || roomId >= 100)
		return 99;
	if (listPrivateChat[roomId].status < 1)
		return 99;
	int i;
	for (i = 0; i < listOnline.size(); i++)
		if (!strcmp(listOnline[i].username, session.username))
			break;
	if (i == listOnline.size())
		return 54;//not online
	int ui1 = listPrivateChat[roomId].user1Idx;
	int ui2 = listPrivateChat[roomId].user2Idx;
	if (ui1 != i && ui2 != i)
		return 99;
	//EnterCriticalSection(&critical);
	listPrivateChat[roomId].status = -1;
	if (ui1 < 0 || ui1 >= listOnline.size() || ui2 < 0 || ui2 >= listOnline.size()) {
		listPrivateChat[roomId].user1Idx = -1;
		listPrivateChat[roomId].user2Idx = -1;
		return 54;
	}
	char endChat[10] = "44";
	listOnline[ui1].status = 0;
	listOnline[ui2].status = 0;
	if (ui1 == i) {
		int ret = send(listOnline[ui2].connSock, endChat, strlen(endChat), 0);
		if (ret == SOCKET_ERROR)
			return 99;
		printf("%s**\n", listOnline[ui2].username);
		return 44;
	}
	else {
		int ret = send(listOnline[ui1].connSock, endChat, strlen(endChat), 0);
		if (ret == SOCKET_ERROR)
			return 99;
		printf("%s**\n", listOnline[ui1].username);
		return 44;
	}
	//LeaveCriticalSection(&critical);
	return 99;
}

int handleCreateRoom(char* data, Session session) {
	int i;
	for (i = 0; i < 100; i++) 
	if (listRoomChat[i].status > -1 && strcmp(listRoomChat[i].roomName, data) == 0){
		return 61;
	}
	for (i = 0; i < 100; i++)
		if (listRoomChat[i].status == -1) {
			int ui = 0;
			for (ui = 0; ui < listOnline.size(); ui++)
				if (!strcmp(listOnline[ui].username, session.username))
					break;
			if (ui == listOnline.size())
				return 16;
			listOnline[ui].status = 1;
			listRoomChat[i].status = 0;
			listRoomChat[i].num = 1;
			strcpy(listRoomChat[i].roomName, data);
			listRoomChat[i].user[0] = ui;
			return 60;
		}
	if (i == 100) {
		printf("Full room");
		return 99;
	}
	return 99;
}

int handleInviteRoom(char* data, Session session) {
	char* room;
	char* user;
	split(data, &room, &user);
	int i;
	if (!strcmp(session.username, user))
		return 64;
	for (i = 0; i < 100; i++)
		if (listRoomChat[i].status > -1 && listRoomChat[i].num > 0 && strcmp(listRoomChat[i].roomName, room) == 0) {
			break;
		}
	if (i == 100)
		return 63;

	int ui = 0;
	for (ui = 0; ui < listOnline.size(); ui++)
		if (!strcmp(listOnline[ui].username, session.username))
			break;
	if (ui == listOnline.size())
		return 16;

	for (ui = 0; ui < listOnline.size(); ui++)
		if (!strcmp(listOnline[ui].username, user))
			break;
	
	if (ui == listOnline.size())
		return 64;

	if (listOnline[ui].status == 1)
		return 66;
	//printf("4-%s-%s\n", room, user);
	char sendBuff[BUFF_SIZE];
	strcpy(sendBuff, "INVITE_ROOM ");
	strcat(sendBuff, listRoomChat[i].roomName);
	strcat(sendBuff, " ");
	strcat(sendBuff, session.username);
	int ret = send(listOnline[ui].connSock, sendBuff, strlen(sendBuff), 0);
	if (ret == SOCKET_ERROR)
		printf("invite room chat fail with code: %d \n", WSAGetLastError());
	return -1;
}

int handleResponseRoom(char* data, Session session) {
	char* room;
	char* accept;
	split(data, &room, &accept);
	int i;
	for (i = 0; i < 100; i++)
		if (listRoomChat[i].status > -1 && listRoomChat[i].num > 0 && !strcmp(listRoomChat[i].roomName, room)) {
			break;
		}
	if (i == 100)
		return 63;

	int ui = 0;
	for (ui = 0; ui < listOnline.size(); ui++)
		if (!strcmp(listOnline[ui].username, session.username))
			break;
	if (ui == listOnline.size())
		return 16;

	if (!strcmp(accept, "1")) {//accept;
		listRoomChat[i].status = 1;
		listRoomChat[i].user[listRoomChat[i].num] = ui;
		listRoomChat[i].num++;
		listOnline[ui].status = 1;
		char sendBuff[BUFF_SIZE];
		strcpy(sendBuff, "NEW_MEM ");
		strcat(sendBuff, session.username);
		for (int j = 0; j < listRoomChat[i].num; j++) {
			int userIdx = listRoomChat[i].user[j];
			if (userIdx < 0 || userIdx >= listOnline.size())
				continue;
			int ret = send(listOnline[userIdx].connSock, sendBuff, strlen(sendBuff), 0);
		}
		return -1;
	}
	else {
		listOnline[ui].status = 0;
		return -1;
	}
	return 99;
}

int handleOutRoom(char* data, Session session) {
	int i;
	for (i = 0; i < 100; i++)
		if (listRoomChat[i].status > -1 && listRoomChat[i].num > 0 && !strcmp(listRoomChat[i].roomName, data)) {
			break;
		}
	if (i == 100)
		return 63;

	int ui = 0;
	for (ui = 0; ui < listOnline.size(); ui++)
		if (!strcmp(listOnline[ui].username, session.username))
			break;
	if (ui == listOnline.size())
		return 16;

	for (int j = 0; j < listRoomChat[i].num; j++)
		if (listRoomChat[i].user[j] == ui) {
			listOnline[ui].status = 0;
			listRoomChat[i].num--;
			
			for (int t = j; t < listRoomChat[i].num; t++)
				listRoomChat[i].user[t] = listRoomChat[i].user[t + 1];

			char sendBuff[BUFF_SIZE];
			strcpy(sendBuff, "OUT_MEM ");
			strcat(sendBuff, session.username);

			for (int t = 0; t < listRoomChat[i].num; t++){
				int userIdx = listRoomChat[i].user[t];
				if (userIdx < 0 || userIdx >= listOnline.size())
					continue;
				int ret = send(listOnline[userIdx].connSock, sendBuff, strlen(sendBuff), 0);
			}
			if (listRoomChat[i].num == 0) {
				listRoomChat[i].status = -1;
				strcpy(listRoomChat[i].roomName, "");
			}
			break;
		}
	return 80;
}

int handleSendRoom(char* data, Session session) {
	char *room, *message;
	split(data, &room, &message);
	int i;
	for (i = 0; i < 100; i++)
		if (listRoomChat[i].status > -1 && listRoomChat[i].num > 0 && !strcmp(listRoomChat[i].roomName, room)) {
			break;
		}
	if (i == 100)
		return 63;

	int ui = 0;
	for (ui = 0; ui < listOnline.size(); ui++)
		if (!strcmp(listOnline[ui].username, session.username))
			break;
	if (ui == listOnline.size())
		return 16;

	char sendBuff[BUFF_SIZE];
	strcpy(sendBuff, "SEND_ROOM ");
	strcat(sendBuff, session.username);
	strcat(sendBuff, " ");
	strcat(sendBuff, message);

	for (int j = 0; j < listRoomChat[i].num; j++){
		int userIdx = listRoomChat[i].user[j];
		if (userIdx < 0 || userIdx >= listOnline.size() || userIdx == ui)
				continue;
		int ret = send(listOnline[userIdx].connSock, sendBuff, strlen(sendBuff), 0);
	}
	return -1;
}
/**
*	@handleMessage: return the code when user send message
*	@param str: message
*   @param session: a session between a client and server
*	@return: a number
**/
int handleMessage(char* str, Session &session) {
	//printf("*%s*\n", session.username);
	if (!strcmp(str, "GET_USER")) {
		return sendListUserOnline(session.connSock);
	}
	int res = 0;
	char* header;
	char* data;
	split(str, &header, &data);
	if (!strcmp(header, "LOGIN")) {
		res = handleLogin(data, session);
	}
	else if (!strcmp(header, "REGISTER")) {
		res = handleRegister(data, session);
	}
	else if (!strcmp(header, "BYE")) {
		res = handleLogout(session);
	}
	else if (!strcmp(header, "REQUEST_CHAT")) {
		res = handleRequestChat(data, session);
	}
	else if (!strcmp(header, "RESPONSE_CHAT")) {
		res = handleResponseChat(data, session);
	}
	else if (!strcmp(header, "SEND")) {
		res = handleSendChat(data, session);
	}
	else if (!strcmp(header, "END_CHAT")) {
		res = handleEndChat(data, session);
	}
	else if (!strcmp(header, "CREATE_ROOM")) {
		res = handleCreateRoom(data, session);
	}
	else if (!strcmp(header, "OUT_ROOM")) {
		res = handleOutRoom(data, session);
	}
	else if (!strcmp(header, "INVITE_ROOM")) {
		res = handleInviteRoom(data, session);
	}
	else if (!strcmp(header, "RESPONSE_ROOM")) {
		res = handleResponseRoom(data, session);
	}
	else if (!strcmp(header, "SEND_ROOM")) {
		res = handleSendRoom(data, session);
	}
	else {
		res = 99;
	}
	return res;
}

int sendListUserOnline(SOCKET connect) {
	char buff[BUFF_SIZE];
	strcpy(buff, "ONLINE ");
	for (int i = 0; i < listOnline.size(); i++) {
		strcat(buff, listOnline[i].username);
		strcat(buff, " ");
	}
	int ret = send(connect, buff, strlen(buff), 0);
	if (ret == SOCKET_ERROR) {
		printf("Send list online users fail with code: %d\n", WSAGetLastError());
		return -1;
	}
	return 30;
}
