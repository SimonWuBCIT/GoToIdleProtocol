#pragma once

#include <Windows.h>
#include <vector>
#include "counterStruct.h"

struct comInputStruct {
	counterStruct counter;

	HANDLE fhand;
	std::vector<std::string> frames;
	HANDLE hFrameRecieved;

	bool* connected;

	comInputStruct() {
		hFrameRecieved = CreateEvent(0, TRUE, 0, 0);
	}

};