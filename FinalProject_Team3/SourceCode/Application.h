#pragma once
#include <vector>
#include <string>
#include <windows.h>
#include <stdio.h>
#include "menu.h"
#include <iostream>
#include "Session.h"
#include "Physical.h"
#include <fstream>
#include <sys/stat.h>

#include "dataEventStruct.h"

constexpr int COM_PORT_MAX = 256;
constexpr int COM_NAME_BUFFER = 32;
constexpr int CHARACTER_WIDTH = 10;


std::vector<int> findValidComPorts();
void fillConnectionMenu();
void initOpenFileName();
DWORD WINAPI saveFileLoop(LPVOID n);
DWORD WINAPI statisticThread(LPVOID n);
void enableMenu();
std::ofstream prepareWrite();
void printToFile(std::ofstream& out_file, const char* buffer, size_t buffer_size);