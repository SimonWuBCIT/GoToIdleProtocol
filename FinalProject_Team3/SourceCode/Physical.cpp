/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: Physical.cpp
--
-- PROGRAM: Final Project
--
-- FUNCTIONS:
-- void writeComPort(HANDLE fhand, std::string, int olen);
-- int getFrameFromCom(comInputStruct&, char* rBuffer, int& rlen, long& timeout, HANDLE);
-- void removeCharFromFront(char* arr, const int size, const unsigned int amount);
-- DWORD WINAPI readComLoop(LPVOID n);
--
-- DATE: December 2, 2008
--
-- REVISIONS:	December 2, 2018
--					-created function: removeCharFromFront
--					-updated function: getFrameFromCom, readComLoop
--				November 26, 2018
--					-created function: writeComPort, getFrameFromCom, 
--				November 24, 2018
--					-created function: readComLoop
--
-- DESIGNER: Phat Le, Cameron Roberts, Simon Wu, Jacky Li
--
-- PROGRAMMER: Phat Le, Cameron Roberts, Simon Wu, Jacky Li
--
-- NOTES:
-- An class to write and read COM port data. Add frames/character to buffer to be used by upper layers.
----------------------------------------------------------------------------------------------------------------------*/
#include "Physical.h"


OVERLAPPED wol = { 0 };

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: writeComPort
--
-- DATE: November 26, 2018
--
-- REVISIONS:	November 26, 2018
--					-Add write file event handling
--
-- DESIGNER: Cameron Roberts
--
-- PROGRAMMER: Cameron Roberts
--
-- INTERFACE: void writeComPort (HANDLE fhand, char c)
--				1) HANDLE fhand: A file handle for the COM port to write to
--              2) char c: The character to wwrite to the COM port
--
-- RETURNS: void
--
-- NOTES:
-- Taken from Assignment 1.
-- This function will loop as long as isConnected return true, reading from the given COM port and passing the 
-- resulting strings to printToScreen to be displayed.
----------------------------------------------------------------------------------------------------------------------*/
void writeComPort(HANDLE fhand, std::string s, int olen) {
	wol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	WriteFile(fhand, s.c_str(), olen, NULL, &wol);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: getFrameFromCom
--
-- DATE: December 2, 2018
--
-- REVISIONS: December 2, 2018
--				-update timing for clock after being moved around.
--				-edit the comInputStruct that is passed in and how it is handled (read)
--			  November 26, 2018
--				-initialized program.
--
-- DESIGNER: Cameron Roberts, Simon Wu, Phat Le, Jacky L1, 
--
-- PROGRAMMER: Cameron Roberts, Simon Wu
--
-- INTERFACE: int getFrameFromCom(comInputStruct& comInput, char* rBuffer, int& rlen, long& timeout, HANDLE evnt)
--				1) comInputStruct& comInput:	struct that contains a vector of strings that holds the read frames
--				2) char* rBuffer:				pointer to buffer array
--				3) int& rlen:					length of buffer
--				4) long& timeout:				milliseconds timeout for reading a frame
--				5) HANDLE evnt:					event handle to trigger to read frames
--
-- RETURNS: int
--				1: Frame received, placed in buffer
--				2: Passed in event triggered
--				0: Timeout/FAIL
--
-- NOTES:
--	Removes a specified number of characters from the front of a character array
----------------------------------------------------------------------------------------------------------------------*/
int getFrameFromCom(comInputStruct& comInput, char* rBuffer, int& rlen, long& timeout, HANDLE evnt) {
	auto initialTime = std::chrono::high_resolution_clock::now();

	int events = 1;
	if (evnt != NULL) {
		OutputDebugString("Second getFrame event found\n");
		events = 2;
	}

	int cRec = 0;

	DWORD eventType;

	const HANDLE handles[2] = { comInput.hFrameRecieved, evnt };
	DWORD returnValue = WaitForMultipleObjects(events, handles, false, timeout);

	auto now_time = std::chrono::high_resolution_clock::now();
	auto time_duration = std::chrono::duration_cast<std::chrono::milliseconds>(now_time - initialTime);
	timeout -= time_duration.count();
	printStringNumber("Timeout : ", timeout);
	switch (returnValue) {
	case WAIT_OBJECT_0:
	{
		ResetEvent(comInput.hFrameRecieved);

		std::copy(comInput.frames[0].begin(), comInput.frames[0].end(), rBuffer);
		rlen = comInput.frames[0].size();
		rBuffer[comInput.frames[0].size()] = '\0';
		comInput.frames.erase(comInput.frames.begin());
		if (comInput.frames.size() != 0) {
			SetEvent(comInput.hFrameRecieved);
		}
		return 1;
	}
	case WAIT_OBJECT_0 + 1:
		return 2;
		break;
	case WAIT_TIMEOUT:
		return 0;
	case WAIT_FAILED:
		OutputDebugString("getComm failed\n");
		printNumber(GetLastError());
		return 0;
	default:
		break;
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: readComLoop
--
-- DATE: December 2, 2018
--
-- REVISIONS:	December 2, 2018
--					-Add SetEvent to print file sending and receiving result to window.
--				November 24, 2018
--					-initialized program.
--
-- DESIGNER: Cameron Roberts, Simon Wu
--
-- PROGRAMMER: Cameron Roberts, Simon Wu
--
-- INTERFACE: DWORD WINAPI readComLoop(LPVOID n)
--				1) LPVOID n : cast to comInputStruct: struct that contains a vector of strings that holds the read frames
--
-- RETURNS: DWORD WINAPI
--
-- NOTES:
--	Thread function to read from a COM port 
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI readComLoop(LPVOID n) {
	comInputStruct* comInput = (comInputStruct*)n;
	char cRecieved[FRAME_SIZE*2];
	int cRec = 0;

	LPVOID buff[BUFFER_SIZE];
	DWORD nRead = 0;

	bool gotSYN = false;

	while (*comInput->connected) {
		OVERLAPPED ol = { 0 };
		ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		if (ReadFile(comInput->fhand, buff, FRAME_SIZE, NULL, &ol) == 0) {}

		bool gotinput = false;

		while (*comInput->connected && !gotinput) {
			DWORD returnValue = WaitForSingleObject(ol.hEvent, INFINITE);

			switch (returnValue) {
			case WAIT_OBJECT_0:
				gotinput = true;
				if (ResetEvent(ol.hEvent) == 0) {}
				if (GetOverlappedResult(comInput->fhand, &ol, &nRead, false) == 0) {}

				if (nRead > 0) {
					printNumber(nRead);
					int tempRec = cRec;

					if (((char*)buff)[0] == SYN && cRec > 3) {

							OutputDebugString("got a syn?\n");
							gotSYN = true;
					}

					for (size_t i = 0; i < nRead; i++)
					{
						cRecieved[tempRec + i] = ((char*)buff)[i];
						printStringNumber("Character Reciieved: ",(int)(((char*)buff)[i]));
						
						cRec++;
					}
				}
				break;
			case WAIT_TIMEOUT:
				break;
			case WAIT_FAILED:
				break;
			default:
				break;
			}

			if (cRecieved[0] == SYN && cRecieved[1] != DC1 && cRecieved[1] != DC2 && cRec >= CONTROL_FRAME_SIZE) {
				std::string str(cRecieved, CONTROL_FRAME_SIZE);
				removeCharFromFront(cRecieved, cRec, CONTROL_FRAME_SIZE);
				cRec -= CONTROL_FRAME_SIZE;

				comInput->frames.push_back(str);
				SetEvent(comInput->hFrameRecieved);
			}
			if (cRecieved[0] == SYN && (cRecieved[2] != DC1 || cRecieved[2] != DC2) && cRec >= FRAME_SIZE) {
				std::string str(cRecieved, FRAME_SIZE);
				removeCharFromFront(cRecieved, cRec, FRAME_SIZE);
				cRec -= FRAME_SIZE;

				comInput->frames.push_back(str);
				SetEvent(comInput->hFrameRecieved);
			}

			if (cRec != 0 && cRecieved[0] != SYN) {
				int i = 0;
				for (; i < cRec; i++) {
					if (cRecieved[i] == SYN) {
						break;
					}
				}
				removeCharFromFront(cRecieved, cRec, i);
				cRec -= i;
			}
			if (cRec != 0 && gotSYN) {
				gotSYN = false;
				int i = 1;
				for (; i < cRec; i++) {
					if (cRecieved[i] == SYN) {
						break;
					}
				}
				printStringNumber("deleted from buffer : ", i);
				removeCharFromFront(cRecieved, cRec, i);
				cRec -= i;
			}
		}
	}
	return 1;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: removeCharFromFront
--
-- DATE: December 2, 2018
--
-- DESIGNER: Phat Le, Jacky Li
--
-- PROGRAMMER: Phat Le, Jacky Li
--
-- INTERFACE: void removeCharFromFront(char* arr, const int size, const unsigned int amount)
--				1) char* arr:					pointer to the char array
--				2) const int size:				size of the array
--				3) const unsigned int amount:	amount of chars to remove
--
-- RETURNS: void
--
-- NOTES:
--	Moves a certain amount of characters to the front of the array
----------------------------------------------------------------------------------------------------------------------*/
void removeCharFromFront(char* arr, const int size, const unsigned int amount) {
	std::copy(arr + amount, arr + size, arr);
	for (int i = size - amount; i < size; i++) {
		arr[i] = 0;
	}
}
