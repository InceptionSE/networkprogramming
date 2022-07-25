#pragma once
#include <WinSock2.h>
#include<stdio.h>
struct RoomChat
{
	char roomName[100];
	int user[100];//mang cac user, moi phan tu la 1 index cua user trong listOnline
	int status = -1;//-1: chua su dung, >-1: dang su dung
	int num = 0;//so user trong phong chat hien tai
};