// TCP_client.cpp : Defines the entry point for the console application.
#include "stdafx.h"
#include<string.h>
#include<stdlib.h>
#include<winsock2.h>
#include<WS2tcpip.h>
#include<conio.h>
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
void getUserOnline(SOCKET client);
void requestRegister(char* buff, char* username, char* password);
void requestLogout(SOCKET client);
void requestLogin(char* buff, char* username, char* password);
int handleMainChoices(int c, SOCKET socket);
void printMenu();
unsigned __stdcall echoThread(void *param);

int status = 0;
char partnerName[20];
char curRoom[5];
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

	// Communicate with server
	int ret, messageLen;
	while (1) {
		char input;
		char buff[BUFF_SIZE], username[BUFF_SIZE], password[BUFF_SIZE];
		string pass;
		//Send message
		printMenu();
		cin >> input;
		//clear cache
		int a;
		while ((a = getchar()) != '\n'); (stdin);
		switch (input) {
		case '1': {
			requestLogin(buff, username, password);
			break;
		}
		case '2': {
			requestRegister(buff, username, password);
			break;
		}
		case '3': {
			cout << "Exit App.\n";
			// Close socket
			closesocket(client);
			// Terminate Winsock
			WSACleanup();
			return 0;
			//break;
		}

		default: {
			strcpy(buff, "");
			cout << "Invalid message! Please choose again.";
			break;
		}
		}
		messageLen = strlen(buff);
		if (messageLen > 0) {
			strcat(buff, "\r\n");
			ret = send(client, buff, strlen(buff), 0);
			if (ret == SOCKET_ERROR)
				printf("Error %d: Cannot send data.", WSAGetLastError());

			//Receive echo message
			ret = recv(client, buff, BUFF_SIZE, 0);
			if (ret == SOCKET_ERROR) {
				if (WSAGetLastError() == WSAETIMEDOUT)
					printf("Time-out!");
				else printf("Error %d: Cannot receive data.", WSAGetLastError());
			}
			else if (strlen(buff) > 0) {
				buff[ret] = 0;
				showResult(chartoInt(buff));
				/////////////////Dang nhap thanh cong
				if (chartoInt(buff) == 10) {
					status = 1;
					printMenu();
					_beginthreadex(0, 0, echoThread, (void *)&client, 0, 0); //start thread
					while (status > 0)
					{
						int ret = recv(client, buff, BUFF_SIZE, 0);
						if (ret == SOCKET_ERROR) {
							// Close socket
							closesocket(client);
							// Terminate Winsock
							WSACleanup();
							return 0;
							//if (WSAGetLastError() == WSAETIMEDOUT)
							//	printf("Time-out!");
							//else printf("Error %d: Cannot receive data.", WSAGetLastError());
						}
						else if (ret > 0) {
							buff[ret] = 0;
							//handleReceive(buff);
							if (strlen(buff) > 2) {
								//cout << endl << buff;

								char* header;
								char* data;
								split(buff, &header, &data);
								if (!strcmp(header, "REQUEST_CHAT")) {
									status = 2;
									strcpy(partnerName, data);
									printf("You have request chat from user '%s'\n", data);
								}
								else if (!strcmp(header, "RESPONSE_CHAT")&&(data[0]=='1')) {
									status = 3;
									printf("You are in Room chat %s - Enter `EXIT` to end chat\n", data);
									strcpy(curRoom, data);
								}
								
								else if (!strcmp(header, "SEND")) {
									printf("%s\n", data);
									if (!strcmp(data, "EXIT")) status = 1;
								}
								else if (!strcmp(header, "GET_USER")) {
									printf("---%s\n", data);
								}
							}
							else {
								showResult(chartoInt(buff));
							}
						}
						printMenu();
					}
				}
			}
		}
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

	while (1) {
		gets_s(input,BUFF_SIZE);
		switch (status)
		{
		case 1:
			temp = handleMainChoices(chartoInt(input), connectedSocket);
			if (temp == -1) {
				shutdown(connectedSocket, SD_SEND);
				closesocket(connectedSocket);
				return 0;
			}
			break;
		case 2://response chat invitation
			char resChat[BUFF_SIZE];
			strcpy(resChat, "RESPONSE_CHAT ");
			strcat(resChat, partnerName);
			if (input[strlen(input) - 1] == toascii('y')) {
				strcat(resChat, " 1\r\n");
			}
			else if (input[strlen(input) - 1] == toascii('n')) {
				strcat(resChat, " 0\r\n");
				status = 1;
			}
			ret = send(connectedSocket, resChat, strlen(resChat), 0);
			if (ret == SOCKET_ERROR)
				printf("Send response chat fail with code: %d \n", WSAGetLastError());
			break;
		case 3://chatting
			char chatBuf[BUFF_SIZE];
			
				strcpy(chatBuf, "SEND ");
				strcat(chatBuf, curRoom);
				strcat(chatBuf, " ");
				strcat(chatBuf, input);
				strcat(chatBuf, "\r\n");
				if (!strcmp(input, "EXIT")) status = 1;


			//printf("--%s--\n", chatBuf);
			ret = send(connectedSocket, chatBuf, strlen(chatBuf), 0);
			if (ret == SOCKET_ERROR)
				printf("Send chat message fail with code: %d \n", WSAGetLastError());
			printMenu();
			break;
		default:
			break;
		}
	}
	shutdown(connectedSocket, SD_SEND);
	closesocket(connectedSocket);
	return 0;
}

void printMenu() {
	switch (status) {
	case 0://chua dang nhap
		   //cout << "\n^^^^^^ ----Login Page---- ^^^^^^";
		cout << "\n----------------\nSelect option:\n1.Login\n2.Register\n3.Exit\n";
		cout << ">>>> ";
		break;
	case 1://Menu chinh
		   //cout << "\n^^^^^^ ----Home Page---- ^^^^^^";
		cout << "\n----------------\nSelect Option:\n1.Get list user online\n2.ChatPrivate\n3.GroupChat\n4.Logout\n5.Exit\n";
		cout << ">>>> ";
		break;
	case 2://Tra loi loi moi chat
		   //cout << "\n^^^^^^ ----Home Page---- ^^^^^^";
		cout << "\n----------------\nDo you accept this? (y/n): ";
		break;
	case 3://Dang chat
		   //cout << "\n^^^^^^ ----Home Page---- ^^^^^^";
		cout << "--- ";
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

int handleMainChoices(int c, SOCKET socket) {
	//Select Option:\n1.Get list user online\n2.ChatPrivate\n3.GroupChat\n4.Logout\n5.Exit
	int res = 1, ret = 0;
	char buff[BUFF_SIZE];
	switch (c) {
	case 1://get online users
		getUserOnline(socket);
		break;
	case 2: //chat private
		cout << "Username: ";
		char username[BUFF_SIZE];
		cin >> username;
		if (strlen(username) > 0) {
			strcpy(buff, "REQUEST_CHAT ");
			strcat(buff, username);
			strcat(buff, "\r\n");
			ret = send(socket, buff, strlen(buff), 0);
			if (ret == SOCKET_ERROR)
				printf("Send request chat fail with code: %d \n", WSAGetLastError());
		}
		
		break;
	case 3:
		cout << "GroupChat: ";
		break;
	case 4: 
		requestLogout(socket);
		break;
	case 5:
		cout << "Exit App";
		closesocket(socket);
		return -1;
	default:
		res = 0;
		cout << ">>>> ";
		break;
	}
	return res;
}
void response(int choice) {

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
		printf("Chat invitation has been declined!\n");
		break;
	}
	case(50): {
		printf("Your request has been sent, please wait user's response\n");
		break;
	}
	case(54): {
		printf("The invited user is offline or not exist!\n");
		break;
	}
	case(55): {
		printf("The invited user is chatting with others!");
	}
	case(99): {
		printf("Undefined message");
		break;
	}
	default:
		printf("UnHandle message");
	}
}

void requestLogin(char* buff, char* username, char* password) {
	int ch;
	string pass;
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
}

void requestLogout(SOCKET client) {
	int ret;
	char buff[BUFF_SIZE];
	strcpy(buff, "BYE\r\n");
	ret = send(client, buff, strlen(buff), 0);
	ret = recv(client, buff, BUFF_SIZE, 0);
	buff[ret] = 0;
	showResult(chartoInt(buff));
}

void requestRegister(char* buff, char* username, char* password) {
	printf("Enter your username: ");
	fflush(stdin);
	cin >> username;
	printf("Enter your password: ");
	cin >> password;
	strcpy(buff, "REGISTER ");
	strcat(buff, username);
	strcat(buff, " ");
	strcat(buff, password);
}

void getUserOnline(SOCKET client) {
	char buff[BUFF_SIZE];
	int ret;
	strcpy(buff, "GET_USER\r\n");
	cout << "\n-----------------User is online-----------------";
	ret = send(client, buff, strlen(buff), 0);
	
}
