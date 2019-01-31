#pragma once

struct counterStruct {
	HANDLE statsEvent;

	int ackCounter;
	int nakCounter;
	int eotCounter;
	int enqCounter;

	int dataFrameResendCounter;

	int sentDataFrameCounter;
	int receivedGoodDataFrameCounter;
	int receivedBadDataFrameCounter;
	int receivedDuplicateDataFrameCounter;

	bool* connected;

	counterStruct() {
		statsEvent = CreateEvent(0, TRUE, 0, 0);
	}
};