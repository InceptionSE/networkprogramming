// TCP_client.cpp : Defines the entry point for the console application.
#include "stdafx.h"
#include<string.h>
#include<stdlib.h>
#include<winsock2.h>
#include<WS2tcpip.h>
#include<conio.h>
#include<stdio.h>
#include"process.h"
#include<iostream>
using namespace std;
#define SERVER_PORT "5500"
#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 2048
#pragma comment(lib,"Ws2_32.lib")

int chartoInt(char *s);
void split(char* str, char** header, char** data);
void showResult(int x);
void getUserOnline(int ret, char buff[], SOCKET client);
int handleMainChoices(int c, SOCKET & socket);
int handleHomeChoices(int c, SOCKET & socket);
void printMenu();
unsigned __stdcall echoThread(void *param);

int status = 0;//0: chua dang nhap, 1: da dang nhap, 2: dang tra loi loi moi chat rieng
//3: dang chat rieng, 4: dang trong phong chat, 5. dang tra loi loi moi vao phong chat 
char partnerName[20];
char curRoom[5];
char roomName[20];

int main(int argc, char* argv[])
{
	// Inittiate WinSock
	WSADATA wsadata;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsadata)) {
		printf("Winsock 2.2 is not supported\n");
		return 0;
	}

	// Construct socket
	SOCKET client;
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client == INVALID_SOCKET) {
		printf("Error %d: Cannot create server socket.", WSAGetLastError());
		return 0;
	}

	// Specify server address
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(chartoInt(SERVER_PORT));
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);

	// Request to connect server
	if (connect(client, (sockaddr *)&serverAddr,
		sizeof(serverAddr))) {
		printf("Error %d: Cannot connect server.", WSAGetLastError());
		return 0;
	}
	printf("Connected server!\n");

	//start thread to get input and send data
	_beginthreadex(0, 0, echoThread, (void *)&client, 0, 0); 

	// receive message from server
	char buff[BUFF_SIZE];
	//int ret, messageLen;
	printMenu();
	while (status > -1) {
		int ret = recv(client, buff, BUFF_SIZE, 0);
		
		if (ret == SOCKET_ERROR) {
			printf("Get message error with code: %d\n", WSAGetLastError());
			break;
		}
		else if (ret > 0) {
			buff[ret] = 0;
			//handleReceive(buff);
			if (ret > 2) {
				//cout << endl << buff;
				char* header;
				char* data;
				split(buff, &header, &data);
				if (!strcmp(header, "REQUEST_CHAT")) {
					status = 2;
					strcpy_s(partnerName, data);
					printf("You have request chat from user '%s'\n", data);
				}
				else if (!strcmp(header, "RESPONSE_CHAT")) {
					if (!strcmp(data, "1")) {//accepted
						status = 3;
						printf("You are in Room chat %s\nSend an empty message to end chat\n", data);
						strcpy_s(curRoom, data);
					}
					else {// rejected
						status = 1;
						printf("Your chat invitation has been rejected\n");
					}
				}
				else if (!strcmp(header, "SEND")) {
					char *user, *mess;
					split(data, &user, &mess);
					printf("<%s>: %s\n", user, mess);
				}
				else if (!strcmp(header, "ONLINE")) {
					printf("Online users: %s\n", data);
				}
				else if (!strcmp(header, "NEW_MEM")) {
					status = 4;
					printf("New member of '%s': %s\n", roomName, data);
				}
				else if (!strcmp(header, "OUT_MEM")) {
					printf("user '%s' out room '%s'\n", data, roomName);
				}
				else if (!strcmp(header, "INVITE_ROOM")) {
					status = 5;
					char *room, *user;
					split(data, &room, &user);
					strcpy(roomName, room);
					printf("You have a invitation to room '%s' from '%s'\n", room, user);
				}
				else if (!strcmp(header, "SEND_ROOM")) {
					char *user, *mess;
					split(data, &user, &mess);
					printf("<%s>: %s\n", user, mess);
				}
			}
			else {
				showResult(chartoInt(buff));
			}
		}
		printMenu();
	}
	closesocket(client);
	WSACleanup();
}
/* echoThread - Thread to receive the message from client and echo*/
unsigned __stdcall echoThread(void *param) {
	//char buff[BUFF_SIZE], buff_send[BUFF_SIZE];
	//string buff_recv = "";
	int ret;
	char input[BUFF_SIZE];
	SOCKET connectedSocket = *(SOCKET*)(param);
	int temp;

	while (status > -1) {
		//cin >> input;
		//printMenu();
		gets_s(input, BUFF_SIZE);
		//printf("--%s--%d--\n", input, status);
		switch (status)
		{
		case 0:
			temp = handleHomeChoices(chartoInt(input), connectedSocket);
			if (temp == -1) {
				status = -1;
				/*shutdown(connectedSocket, SD_SEND);
				closesocket(connectedSocket);
				return 0;*/
			}
			break;
		case 1:
			temp = handleMainChoices(chartoInt(input), connectedSocket);
			if (temp == -1) {
				status = -1;
				/*shutdown(connectedSocket, SD_SEND);
				closesocket(connectedSocket);
				return 0;*/
			}
			break;
		case 2://response chat invitation	
			char resChat[BUFF_SIZE];
			strcpy_s(resChat, "RESPONSE_CHAT ");
			strcat_s(resChat, partnerName);
			if (!strcmp(input, "y")) {
				strcat_s(resChat, " 1\r\n");
			}
			else if (!strcmp(input, "n")) {
				strcat_s(resChat, " 0\r\n");
				status = 1;
				printMenu();
			}
			else {
				printMenu();
				continue;
			}
			ret = send(connectedSocket, resChat, strlen(resChat), 0);
			if (ret == SOCKET_ERROR)
				printf("Send response chat fail with code: %d \n", WSAGetLastError());
			break;
		case 3://chatting
			char chatBuf[BUFF_SIZE];
			if (strlen(input) == 0) {//end chat
				cout << "End chat...!\n";
				strcpy_s(chatBuf, "END_CHAT ");
				strcat_s(chatBuf, curRoom);
				strcat_s(chatBuf, "\r\n");
				ret = send(connectedSocket, chatBuf, strlen(chatBuf), 0);
				if (ret == SOCKET_ERROR)
					printf("Send end chat request fail with code: %d \n", WSAGetLastError());
				else
					status = 1;
			}
			else {
				strcpy_s(chatBuf, "SEND ");
				strcat_s(chatBuf, curRoom);
				strcat_s(chatBuf, " ");
				strcat_s(chatBuf, input);
				strcat_s(chatBuf, "\r\n");
			
				//printf("--%s--\n", chatBuf);
				ret = send(connectedSocket, chatBuf, strlen(chatBuf), 0);
				if (ret == SOCKET_ERROR)
					printf("Send chat message fail with code: %d \n", WSAGetLastError());
			}
			printMenu();
			break;
		case 4: //chat room
			char sendBuff[BUFF_SIZE];
			if (strlen(input) == 0) {//end chat
				cout << "Out room...\n";
				strcpy_s(sendBuff, "OUT_ROOM ");
				strcat_s(sendBuff, roomName);
				strcat_s(sendBuff, "\r\n");
				ret = send(connectedSocket, sendBuff, strlen(sendBuff), 0);
				if (ret == SOCKET_ERROR)
					printf("Send out room fail with code: %d \n", WSAGetLastError());
				else
					status = 1;
			}
			else {
				char* header;
				char* data;
				split(input, &header, &data);
				if (!strcmp(header, "invite")) {//invite users
					strcpy(sendBuff, "INVITE_ROOM ");
					strcat(sendBuff, roomName);
					strcat(sendBuff, " ");
					strcat(sendBuff, data);
					strcat(sendBuff, "\r\n");
				}
				else if (!strcmp(header, "send")) {//invite users
					strcpy(sendBuff, "SEND_ROOM ");
					strcat(sendBuff, roomName);
					strcat(sendBuff, " ");
					strcat(sendBuff, data);
					strcat(sendBuff, "\r\n");
				}
				else {
					printMenu();
					break;
				}
				ret = send(connectedSocket, sendBuff, strlen(sendBuff), 0);
				if (ret == SOCKET_ERROR)
					printf("Send room chat fail with code: %d \n", WSAGetLastError());
				printMenu();
			}
			break;
		case 5://response chat room invitation	
			strcpy_s(resChat, "RESPONSE_ROOM ");
			strcat_s(resChat, roomName);
			if (!strcmp(input, "y")) {
				strcat_s(resChat, " 1\r\n");
			}
			else if (!strcmp(input, "n")) {
				strcat_s(resChat, " 0\r\n");
				status = 1;
				printMenu();
			}
			else {
				printMenu();
				continue;
			}
			ret = send(connectedSocket, resChat, strlen(resChat), 0);
			if (ret == SOCKET_ERROR)
				printf("Send response room chat fail with code: %d \n", WSAGetLastError());
			break;
		default:
			break;
		}
	}
	//shutdown(connectedSocket, SD_SEND);
	//closesocket(connectedSocket);
	return 0;
}

void printMenu() {
	switch (status) {
	case 0://chua dang nhap
		cout << "\n----------------\nSelect option:\n1.Login\n2.Register\n3.Exit\n";
		cout << ">>>> ";
		break;
	case 1://Menu chinh
		cout << "\n----------------\nSelect Option:\n1.Get list user online\n2.ChatPrivate\n3.GroupChat\n4.Logout\n5.Exit\n";
		cout << ">>>> ";
		break;
	case 2://Tra loi loi moi chat
		cout << "\n----------------\nDo you accept this? (y/n): ";
		break;
	case 3://Dang chat
		cout << ">>";
		break;
	case 4://Tao room thanh cong
		cout << ">>";
		break;
	case 5://Tra loi loi moi room chat
		cout << "\n----------------\nDo you accept this? (y/n): ";
		break;
	}
}

void handleReceive(char recvBuff[]) {
	if (strlen(recvBuff) > 2) {
		cout << endl << recvBuff;
	}
	else {
		showResult(chartoInt(recvBuff));
	}
}

int handleMainChoices(int c, SOCKET & socket) {
	//Select Option:\n1.Get list user online\n2.ChatPrivate\n3.GroupChat\n4.Logout\n5.Exit
	int res = 1, ret = 0;
	char buff[BUFF_SIZE];
	switch (c) {
	case 1://get online users
		strcpy_s(buff, "GET_USER\r\n");
		ret = send(socket, buff, strlen(buff), 0);
		if (ret == SOCKET_ERROR)
			printf("Send get users online fail with code: %d \n", WSAGetLastError());
		break;
	case 2: //chat private
		cout << "Username: ";
		char username[BUFF_SIZE];
		cin >> username;
		if (strlen(username) > 0) {
			strcpy_s(buff, "REQUEST_CHAT ");
			strcat_s(buff, username);
			strcat_s(buff, "\r\n");
			ret = send(socket, buff, strlen(buff), 0);
			if (ret == SOCKET_ERROR)
				printf("Send request chat fail with code: %d \n", WSAGetLastError());
		}
		break;
	case 3: //group chat
		cout << "Room Name: ";
		char room[BUFF_SIZE];
		cin >> room;
		if (strlen(room) > 0) {
			strcpy_s(buff, "CREATE_ROOM ");
			strcat_s(buff, room);
			strcat_s(buff, "\r\n");
			ret = send(socket, buff, strlen(buff), 0);
			if (ret == SOCKET_ERROR)
				printf("creat room chat fail with code: %d \n", WSAGetLastError());
			strcpy(roomName, room);
		}
		break;		
	case 4: //logout
		//requestLogout(ret, buff, client);
		strcpy_s(buff, "BYE\r\n");
		ret = send(socket, buff, strlen(buff), 0);
		if (ret == SOCKET_ERROR)
			printf("Send request log out fail with code: %d \n", WSAGetLastError());
		break;	
	case 5: 
		cout << "Exit App";
		//shutdown(socket, SD_SEND);
		//closesocket(socket);
		return -1;
	default: 
		res = 0;
		cout << ">>>> ";
		break;
	}
	return res;
}

int handleHomeChoices(int c, SOCKET & socket) {
	//Select Option:\n1.Login\n2.Register\n3.Exit
	int res = 1, i = 0;
	char buff[BUFF_SIZE];
	string pass;
	char username[20], password[20];
	switch (c) {
	case 1://Login
		int ch;
		printf("Enter your username: ");
		fflush(stdin);
		cin >> username;
		printf("Enter your password: ");
		ch = getch();
		while (ch != 13)
		{
			if (ch != 8 && ((ch > 47 && ch < 58) || (ch >64 && ch < 91) || (ch>96 && ch <123))) {
				pass.push_back(ch);
				cout << "*";
			}
			else if (ch == 8)
			{
				if (pass != "") {
					pass.pop_back();
					cout << "\b \b";
				}
			}
			ch = getch();
		}
		for (int i = 0; i < pass.length(); i++) {
			password[i] = pass[i];
		}
		cout << endl;
		password[pass.length()] = 0;
		strcpy(buff, "LOGIN ");
		strcat(buff, username);
		strcat(buff, " ");
		strcat(buff, password);
		break;
	case 2: //chat private
		cout << "Enter your username: ";
		fflush(stdin);
		cin >> username;
		cout << "Enter your password: ";
		cin >> password;
		strcpy(buff, "REGISTER ");
		strcat(buff, username);
		strcat(buff, " ");
		strcat(buff, password);
		break;
	case 3:
		cout << "Exit App.\n";
		// Close socket
		//shutdown(socket, SD_SEND);
		//closesocket(socket);
		// Terminate Winsock
		//WSACleanup();
		return -1;
		//break;
	default:
		strcpy(buff, "");
		res = 0;
	}
	if (strlen(buff) > 0) {
		strcat(buff, "\r\n");
		int ret = send(socket, buff, strlen(buff), 0);
		if (ret == SOCKET_ERROR)
			printf("Error %d: Cannot send data.", WSAGetLastError());
	}
	return res;
}
/**
*	@function chartoInt: convert a string to number (int)
*
*	@param s: A pointer to a input string
*
*	@return: a number
**/
int chartoInt(char *s)
{
	int res = 0;
	for (int i = 0; i < strlen(s); i++)
		if (s[i] < '0' || s[i] > '9')
			return -1;
		else
		{
			res = res * 10 + (s[i] - '0');
		}
	return res;
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
*	@function showResult: print the message based on the return code
*
*	@param s: a return code
*
*	@return: a string( message)
**/
void showResult(int x) {
	switch (x) {
		case(10): {
			printf("Login successfull\n");
			status = 1;
			break;
		}
		case(13): {
			printf("Password isn't correct!\n");
			break;
		}
		case(11): {
			printf("Account does not exits!\n");
			break;
		}
		case(12): {
			printf("Account is already logged in other client!\n");
			break;
		}
		case(14): {
			printf("You have already logged in!\n");
			break;
		}
		case(15): {
			printf("Logout Successful! See you againnnnn\n");
			status = 0;
			break;
		}
		case(20): {
			printf("Register Successfull\n");
			break;
		}
		case(21): {
			printf("This username has been used!\n");
			break;
		}
		case(22): {
			printf("Your username must be digit or alphabet!\n");
			break;
		}
		case(53): {
			printf("Your chat request has been rejected!\n");
			break;
		}
		case(50): {
			printf("Your chat request has been accepted!\n");
			break;
		}
		case(54): {
			printf("The invited user is offline or not exist!\n");
			break;
		}
		
		case(64): {
			printf("The invited user is offline or not exist!\n");
			break;
		}
		case(60): {
			printf("Create room chat successful. Room name: %s!\n", roomName);
			printf("-----------\ninput empty string to out room\ninvite <usernames>\nsend <message>\n--------------\n");
			status = 4;
			break;
		}
		case(61): {
			printf("Room name '%s' exist!\n", roomName);
			strcpy(roomName, "");
			break;
		}
		case(66): {
			printf("The invited user is in a box chat!\n");
			break;
		}
		case(67): {
			printf("Your invitation has been rejected\n");
			break;
		}
		case(80): {
			printf("You out room '%s'\n", roomName);
			strcpy(roomName, "");
			status = 1;
			break;
		}
		case(44): {
			printf("End conversation!\n");
			if (status == 3)
				status = 1;
			break;
		}
		case(99): {
			printf("Undefined message");
			break;
		}
		default:
			printf("UnHandle message");
	}
}

void getUserOnline(int ret, char buff[], SOCKET client) {
	strcpy(buff, "GET_USER\r\n");
	cout << "\n-----------------User is online-----------------";
	ret = send(client, buff, strlen(buff), 0);
	ret = recv(client, buff, BUFF_SIZE, 0);
	buff[ret] = 0;
	cout << endl << buff;
}

