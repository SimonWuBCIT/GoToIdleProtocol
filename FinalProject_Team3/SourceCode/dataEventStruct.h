#pragma once
#include <vector>
#include <windows.h>
#include "counterStruct.h"

struct dataEventStruct {
	std::vector<std::string> data;
	HANDLE hEvent;

	bool* connected;

	dataEventStruct() {
		hEvent = CreateEvent(0, TRUE, 0, 0);
	}
};