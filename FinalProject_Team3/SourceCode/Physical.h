#pragma once
#include <Windows.h>
#include <stdio.h>
#include <fstream>
#include <thread>
#include <random>
#include "dataEventStruct.h"
#include "PrintUtils.h"
#include "comInputStruct.h"
#include "protocolStruct.h"
#include "idleStruct.h"

constexpr int BUFFER_SIZE = 2056;
constexpr int DATA_SIZE = 1021;
constexpr int CONTROL_FRAME_SIZE = 3;
constexpr int DATA_HEADER_SIZE = 2;
constexpr int CRC_BYTES = 1;
constexpr int FRAME_SIZE = DATA_HEADER_SIZE + DATA_SIZE + CRC_BYTES;



constexpr int MAX_FRAMES = 10;
constexpr int MAX_RESENDS = 6;

/*ASCII CODES*/
constexpr char SYN = 22;
constexpr char EOT = 4;
constexpr char ENQ = 5;
constexpr char ACK = 6;
constexpr char DC1 = 17;
constexpr char DC2 = 18;
constexpr char DC4 = 20;
constexpr char NAK = 21;

void writeComPort(HANDLE fhand, std::string, int olen);
int getFrameFromCom(comInputStruct&, char* rBuffer, int& rlen, long& timeout, HANDLE);
bool waitForLine(comInputStruct&, dataEventStruct* pfd);
void removeCharFromFront(char* arr, const int size, const unsigned int amount);
DWORD WINAPI readComLoop(LPVOID n);