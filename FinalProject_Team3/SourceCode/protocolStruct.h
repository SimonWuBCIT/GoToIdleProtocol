#pragma once
#include "dataEventStruct.h"
#include "comInputStruct.h"
#include "idleStruct.h"
#include "counterStruct.h"

struct protocolStruct {
	dataEventStruct frameData;
	dataEventStruct fileData;
	comInputStruct comInput;
	idleStruct is;
	bool connected;

	protocolStruct() {
		comInput.connected = &connected;
		comInput.counter.connected = &connected;

		frameData.connected = &connected;

		fileData.connected = &connected;

		is.connected = &connected;
	}
};