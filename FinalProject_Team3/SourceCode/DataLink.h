#pragma once
#include "Physical.h"
#include "CRC.h"
#include <vector>
#include <sstream>
#include <fstream>

using std::ifstream;
using std::string;


/*FRAME MASKS*/
constexpr int DATA_FRAME_MASK = 1;
constexpr int ACK0_MASK = 2;
constexpr int ACK1_MASK = 4;
constexpr int ENQ_MASK = 8;
constexpr int EOT_MASK = 16;
constexpr int NAK_MASK = 32;
constexpr int EVENT_MASK = 64;
constexpr int TIMEOUT_MASK = 0;



/*CONTROL FRAME CONSTANTS*/
constexpr char ACK0_FRAME[3] = { SYN, ACK, DC1 };
constexpr char ACK1_FRAME[3] = { SYN, ACK, DC2 };
constexpr char ENQ_FRAME[3] = { SYN, ENQ, DC1 };
constexpr char EOT_FRAME[3] = { SYN, EOT, DC1 };
constexpr char NAK_FRAME[3] = { SYN, NAK, DC1 };


void createMultipleDataFrames(ifstream& inFile, dataEventStruct*);
bool grabData(char* buffer, ifstream& inFile);
void addPadding(int index, char* buffer);
void createReadCommLoop(comInputStruct&);
void sendFrame(comInputStruct& comInput,const char* frame, size_t frameSize);
void sendFrame(idleStruct&, const char* frame, size_t frameSize);
bool checkFrame(const char* frame, const int& rlen);

unsigned int readFrame(comInputStruct&, unsigned int, int, long, char*, int&, HANDLE);
unsigned int maskFromFrame(char*, comInputStruct&);
