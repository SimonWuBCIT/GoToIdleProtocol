#pragma once
#include <Windows.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <bitset>
#include <fstream>
#include "DataLink.h"
#include "dataEventStruct.h"
#include "protocolStruct.h"
#include "Application.h"


/*TIMEOUT CONSTANTS*/
constexpr long CONNECTION_TIMEOUT = 30000;
constexpr long RECEIVE_TIMEOUT = 8000;
constexpr long SEND_TIMEOUT = 2000L;
constexpr long RANDOM_WAIT_MAX = 3000;
constexpr long RANDOM_WAIT_MIN = 1;
constexpr long WAIT_LINE_TIMEOUT = 2000;
constexpr long IDLE_EOT_PERIOD = 500L;



bool createSession(COMMCONFIG&, char* comPort, protocolStruct&);
void closeSession();
void idleState(protocolStruct&);
void createIdleLoop(idleStruct&);
void frameFile(std::string fileName);
DWORD WINAPI idleLoop(LPVOID n);
DWORD WINAPI launchIdleState(LPVOID n);
void receivedState(comInputStruct&, dataEventStruct&);
void sendState(comInputStruct&, dataEventStruct* pfd);