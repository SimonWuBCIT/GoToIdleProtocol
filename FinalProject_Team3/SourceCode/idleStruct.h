#pragma once
#include <Windows.h>
#include "counterStruct.h"

struct idleStruct {
	HANDLE fhand;
	HANDLE hInIdle;

	bool* connected;

	idleStruct() {
		hInIdle = CreateEvent(0, TRUE, 0, 0);
	}
};