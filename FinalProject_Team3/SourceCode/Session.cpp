/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: Session.cpp
--
-- PROGRAM: Final Project
--
-- FUNCTIONS:
-- bool createSession(COMMCONFIG&, char* comPort, protocolStruct&);
-- void closeSession();
-- void idleState(protocolStruct&);
-- void createIdleLoop(idleStruct&);
-- void frameFile(std::string fileName);
-- DWORD WINAPI idleLoop(LPVOID n);
-- DWORD WINAPI launchIdleState(LPVOID n);
-- void receivedState(comInputStruct&, dataEventStruct&);
-- void sendState(comInputStruct&, dataEventStruct* pfd);
--
-- DATE: December 2, 2018
--
-- REVISION:	December 2, 2018
--					-updated function: createSession, frameFile, idleState, waitForLine, receivedState, sendState
--				November 26, 2018
--					-updated function: createSession
--					-created function: launchIdleState 
--				November 24, 2018
--					-created function: createSession, frameFile, idleState, waitForLine, receivedState, sendState,
--									   idleLoop, createIdleLoop
--
-- DESIGNER: Phat Le, Cameron Roberts, Simon Wu, Jacky Li
--
-- PROGRAMMER: Phat Le, Cameron Roberts, Simon Wu, Jacky Li
--
-- NOTES:
-- An class to initialize, maintain, and end a connection to a COM port. Also handles the idle state.
-- Only one connection may be made at a time.
----------------------------------------------------------------------------------------------------------------------*/

#include "Session.h"

HANDLE fhand;

protocolStruct* pps;

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: createSession
--
-- DATE: Deccember 2, 2018
--
-- REVISIONS:	December 2, 2018
--					-modified to take in protocolStruct
--				November 26, 2018
--					-modified to create idle state thread
--				November 24, 2018
--					-initial program with minimal modification.
--
-- DESIGNER: Phat Le, Cameron Roberts, Simon Wu, Jacky Li
--
-- PROGRAMMER: Cameron Roberts
--
-- INTERFACE: bool createSession(COMMCONFIG& cc, char* comPort, protocolStruct& ps) 
--				1) COMMCONFIG& cc: An alias to a COMCONFIG struct. Used to configure the COM port 
--				2) char* comPort: the COM port name
--				3) protocolStruct& ps: the struct that contains all the other struct that holds the
--				   frames received and sent, events relating to them, and whether connection is live
--
-- RETURNS: Returns false if the session could not be created, either because there is already a connection or an
--			error occured setting up the COM port
--
-- NOTES:
-- Taken from Assignment 1.
-- This funciton will use the given COMCONFIG object to setup the COM port and create a thread to launch idle state
----------------------------------------------------------------------------------------------------------------------*/
bool createSession(COMMCONFIG& cc, char* comPort, protocolStruct& ps) {

	pps = &ps;

	if (ps.connected) {
		return false;
	}

	fhand = CreateFile(comPort, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	
	if (!SetCommMask(fhand, EV_RXCHAR)) {
		return false;
	}

	if (!SetupComm(fhand, FRAME_SIZE*2, FRAME_SIZE*2)) {
		return false;
	}

	ps.connected = true;
	ps.comInput.fhand = fhand;
	ps.is.fhand = fhand;


	SetCommConfig(fhand, &cc, cc.dwSize);

	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = 1;
	timeouts.ReadTotalTimeoutConstant = 1;
	timeouts.ReadTotalTimeoutMultiplier = 1;
	timeouts.WriteTotalTimeoutConstant = 1;
	timeouts.WriteTotalTimeoutMultiplier = 2;
	SetCommTimeouts(fhand, &timeouts);

	HANDLE hThrd;
	DWORD threadId;
	
	hThrd = CreateThread(NULL, 0, launchIdleState, &ps, 0, &threadId);
	if (hThrd)
	{
		CloseHandle(hThrd);
	}
	return true;
}


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: launchIdleState
--
-- DATE: November 26th, 2018
--
-- DESIGNER: Phat Le, Cameron Roberts, Simon Wu, Jacky Li
--
-- PROGRAMMER: Cameron Roberts
--
-- INTERFACE: DWORD WINAPI launchIdleState(LPVOID n)
--				1) LPVOID n: cast to protocolStruct
--
-- RETURNS: DWORD WINAPI 
--
-- NOTES:
-- Thread to create idle state.
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI launchIdleState(LPVOID n) {
	idleState(*(protocolStruct*)n);
	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: closeSession
--
-- DATE: November 26, 2018
--
-- DESIGNER: Cameron Roberts
--
-- PROGRAMMER: Cameron Roberts
--
-- INTERFACE: void closeSession (void)
--
-- RETURNS: void.
--
-- NOTES:
-- Shuts this session down, closes the COM port handle, sets global protocolStruct connected status to false, reenables
--	the connect menu item
----------------------------------------------------------------------------------------------------------------------*/
void closeSession() {
	if (pps->connected) {
		CloseHandle(fhand);
		pps->connected = false;
		enableMenu();
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: frameFile
--
-- DATE: December 2, 2018
--
-- REVISIONS:	December 2, 2018
--					-revised to use protocolStruct
--				November 24, 2018
--					-initialized program.
--
-- DESIGNER: Cameron Roberts, Simon Wu
--
-- PROGRAMMER: Cameron Roberts, Simon Wu
--
-- INTERFACE: void frameFile(std::string fileName)
--				1) string fileName: path to file
-- RETURNS: void.
--
-- NOTES:
-- Takes in a file location, creates a filestream, and creates data frames with the ifstream created from the file
-- Calls createMultipleDataFrames with the created ifstream
-- Sets the event inside the global protocolStruct's frameData
----------------------------------------------------------------------------------------------------------------------*/
void frameFile(std::string fileName) {
	ifstream in(fileName);
	createMultipleDataFrames(in, &(pps->frameData));

	SetEvent(pps->frameData.hEvent);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: idleState
--
-- DATE: December 2, 2018
--
-- REVISIONS:	December 2, 2018
--					-modified to take in protocolStruct
--				November 24, 2018
--					-initialized program.
--
-- DESIGNER: Cameron Roberts, Simon Wu
--
-- PROGRAMMER: Cameron Roberts, Simon Wu
--
-- INTERFACE: void idleState(protocolStruct& ps)
--				1) protocolStruct& ps: the struct that contains all the other struct that holds the
--				   frames received and sent, events relating to them, and whether connection is live
-- RETURNS: void.
--
-- NOTES:
-- idle state.
----------------------------------------------------------------------------------------------------------------------*/
void idleState(protocolStruct& ps) {

	createIdleLoop(ps.is);

	createReadCommLoop(ps.comInput);

	bool needToWait = false;

	while (ps.connected) {
		SetEvent(ps.is.hInIdle);
		OutputDebugString("while loop start\n");
		char rBuffer[BUFFER_SIZE];
		int rlen = 0;
		int frameMask = 0;

		if (needToWait) {
			needToWait = false;

			std::random_device rd{};
			std::mt19937 engine{ rd() };
			std::uniform_int_distribution<int> dist{ RANDOM_WAIT_MIN,RANDOM_WAIT_MAX };
			int randomWait = dist(engine);

			frameMask = readFrame(ps.comInput, ENQ_MASK, 1, randomWait, rBuffer, rlen, NULL);

			if (frameMask == ENQ_MASK){
				ResetEvent(ps.is.hInIdle);
				sendFrame(ps.comInput, ACK0_FRAME, CONTROL_FRAME_SIZE);
				receivedState(ps.comInput, ps.fileData);
			}
		}
		else {
			frameMask = readFrame(ps.comInput, ENQ_MASK | EOT_MASK , 1, CONNECTION_TIMEOUT, rBuffer, rlen, ps.frameData.hEvent);
			printStringNumber("frame mask = ", frameMask);
			switch (frameMask) {
			case ENQ_MASK:
				ResetEvent(ps.is.hInIdle);
				sendFrame(ps.comInput, ACK0_FRAME, CONTROL_FRAME_SIZE);
				receivedState(ps.comInput, ps.fileData);
				break;
			case EVENT_MASK:
				ResetEvent(ps.is.hInIdle);
				sendFrame(ps.comInput, ENQ_FRAME, CONTROL_FRAME_SIZE);
				waitForLine(ps.comInput, &(ps.frameData));
				needToWait = true;
				break;
			case EOT_MASK:
				break;
			default:
				OutputDebugString("NO LONGER CONNECETEDESRNGDSNSKLFGXhbNT");
				closeSession();
			}
		}
	}
	
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: waitForLine
--
-- DATE: December 2nd, 2018
--
-- REVISIONS:	December 2, 2018
--					-modified to use structs comInputStruct, dataEventStruct
--				November 24, 2018
--					-initialized program.
--
-- DESIGNER: Cameron Roberts, Simon Wu
--
-- PROGRAMMER: Cameron Roberts, Simon Wu
--
-- INTERFACE: bool waitForLine(comInputStruct& comInput, dataEventStruct *pointerFrameData)
--				1) comInputStruct& comInput: struct that contains a vector of strings that holds the read frames,
--				   the event to trigger reading from COM port, and flag to indicate live connection.
--				2) dataEventStruct *pointerFrameData: struct that contains the data buffer that will be continuously
--					sent to the COM port, and the event handle that will be triggered when data is ready to be sent.
--
-- RETURNS: False if we received an ENQ, true otherwise
--
-- NOTES:
-- State.
-- Reads the incoming frame, and deciphers which type of frame it is - act accordingly
--	If ACKs received, go to send state
--	If ENQ received, returns false
--	If timeout (default), returns true
----------------------------------------------------------------------------------------------------------------------*/
bool waitForLine(comInputStruct& comInput, dataEventStruct *pointerFrameData) {
	char rBuffer[BUFFER_SIZE];
	int rlen = 0;
	OutputDebugString("Start waiting for ACK");

	int frameMask = readFrame(comInput, ACK0_MASK | ACK1_MASK | ENQ_MASK, 1, WAIT_LINE_TIMEOUT, rBuffer, rlen, NULL);
	printStringNumber("RECIEVED MASK WAITING FOR ACK: ", frameMask);

	switch (frameMask) {
	case ACK0_MASK:
		OutputDebugString("-----Got ACK-----");
		sendState(comInput, pointerFrameData);
		break;
	case ACK1_MASK:
		OutputDebugString("-----Got ACK-----");
		sendState(comInput, pointerFrameData);
		break;
	case ENQ_MASK:
		OutputDebugString("-----Got ENQ-----");
		return false;
	default:
		OutputDebugString("-----Got ACK timeout-----");
		break;
	}
	return true;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: receivedState
--
-- DATE: December 2nd, 2018
--
-- REVISIONS:	December 2, 2018
--					-modified to take in comInputStruct, dataEventStruct
--				November 24, 2018
--					-initialized program.
--
-- DESIGNER: Cameron Roberts, Simon Wu, Jacky Li, Phat Le
--
-- PROGRAMMER: Cameron Roberts, Simon Wu
--
-- INTERFACE: void receivedState(comInputStruct& comInput, dataEventStruct& pointerFileData)
--				1) comInputStruct& comInput: struct that contains a vector of strings that holds the read frames,
--				   the event to trigger reading from COM port, and flag to indicate live connection.
--
--				2) dataEventStruct& pointerFileData: struct that contains the data buffer that will be continuously
--					sent to the COM port, and the event handle that will be triggered when data is ready to be sent.
--
-- RETURNS: False if we received an ENQ, true otherwise
--
-- NOTES:
--	receive State, when the line is conceded to the other side
--	Deciphers if frame received is a data or EOT, acts accordingly
----------------------------------------------------------------------------------------------------------------------*/
void receivedState(comInputStruct& comInput, dataEventStruct& pointerFileData) {
	bool transmissionEnd = false;

	char lastDC = DC2;

	//Wait for data frame
	while (!transmissionEnd) {
		char rBuffer[BUFFER_SIZE];
		int rlen = 0;
		int frameMask = readFrame(comInput, DATA_FRAME_MASK | EOT_MASK, 1, RECEIVE_TIMEOUT, rBuffer, rlen, NULL);
		switch (frameMask) {
		case DATA_FRAME_MASK:
			OutputDebugString("DATA FRAME MASK \n");
			if (!checkFrame(rBuffer, rlen)) {
				OutputDebugString("B A D  frame \n");
				sendFrame(comInput, NAK_FRAME, CONTROL_FRAME_SIZE);
				comInput.counter.receivedBadDataFrameCounter++;
				SetEvent(comInput.counter.statsEvent);
				break;
			}

			if (rBuffer[1] != lastDC) {
				lastDC = rBuffer[1];
				std::string temp(rBuffer + DATA_HEADER_SIZE, DATA_SIZE);
				pointerFileData.data.emplace_back(temp);
				printStringNumber("amount of things i want you to print: ", pointerFileData.data.size());

				SetEvent(pointerFileData.hEvent);
				OutputDebugString("SET WRITE TO FILE EVENT\n");

				comInput.counter.receivedGoodDataFrameCounter++;
			}
			else {
				comInput.counter.receivedDuplicateDataFrameCounter++;
			}

			{
				char ack[]{ SYN, ACK, lastDC };
				sendFrame(comInput, ack, CONTROL_FRAME_SIZE);
			}
			SetEvent(comInput.counter.statsEvent);
			break;
		case EOT_MASK:
			OutputDebugString("got EOT");
			transmissionEnd = true;
			break;
		default:
			OutputDebugString("?DID WE TIMEOUT?");
			transmissionEnd = true;
			break;
		}
	}
	OutputDebugString("RETURNING TO IDLE FROM RECEIVED STATE");
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: sendState
--
-- DATE: December 2nd, 2018
--
-- REVISIONS:	December 2, 2018
--					-modified to take in comInputStruct, dataEventStruct
--				November 24, 2018
--					-initialized program.
--
-- DESIGNER: Cameron Roberts, Simon Wu, Jacky Li, Phat Le
--
-- PROGRAMMER: Cameron Roberts, Simon Wu
--
-- INTERFACE: void sendState(comInputStruct& comInput, dataEventStruct* pointerFileData)
--				1) comInputStruct& comInput: struct that contains a vector of strings that holds the read frames,
--				   the event to trigger reading from COM port, and flag to indicate live connection.
--
--				2) dataEventStruct* pointerFileData: struct that contains the data buffer that will be continuously
--					sent to the COM port, and the event handle that will be triggered when data is ready to be sent.
--
-- RETURNS: False if we received an ENQ, true otherwise
--
-- NOTES:
--	send State, when the line is obtained
--	Sends file data (pointerFileData )to be sent to the COM port (comInput), continues sending if 
--		ACK frames received matches
--		else resend frame
----------------------------------------------------------------------------------------------------------------------*/
void sendState(comInputStruct& comInput, dataEventStruct* pointerFileData) {
	int framesSent = 0;
	int resends = 0;
	while (framesSent < MAX_FRAMES && resends <= MAX_RESENDS) {
		if (pointerFileData->data.empty()) {
			ResetEvent(pointerFileData->hEvent);
			break;
		}
		printStringNumber("dataFrames left to send", pointerFileData->data.size());

		pointerFileData->data[0][1] = DC1 + framesSent % 2;

		sendFrame(comInput, pointerFileData->data[0].c_str(), FRAME_SIZE);

		char rBuffer[BUFFER_SIZE];
		int rlen = 0;
		int frameMask = readFrame(comInput, ACK0_MASK | ACK1_MASK | NAK_MASK, 1, SEND_TIMEOUT, rBuffer, rlen, NULL);
		switch (frameMask) {
		case ACK0_MASK:
			resends = 0;
			framesSent++;
			pointerFileData->data.erase(pointerFileData->data.begin());
			comInput.counter.sentDataFrameCounter++;
			SetEvent(comInput.counter.statsEvent);
			break;
		case ACK1_MASK:
			resends = 0;
			framesSent++;
			pointerFileData->data.erase(pointerFileData->data.begin());
			comInput.counter.sentDataFrameCounter++;
			SetEvent(comInput.counter.statsEvent);
			break;
		case NAK_MASK:
			//cascade to default
		default:
			resends++;
			comInput.counter.dataFrameResendCounter++;
			SetEvent(comInput.counter.statsEvent);
			break;
		}
	}
	OutputDebugString("Leaving send state\n");
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: idleLoop
--
-- DATE: November 24th, 2018
--
-- DESIGNER: Cameron Roberts, Simon Wu, Jacky Li, Phat Le
--
-- PROGRAMMER: Cameron Roberts
--
-- INTERFACE: DWORD WINAPI idleLoop(LPVOID n)
--				1) LPVOID n : cast to idleStruct*
--
-- RETURNS: DWORD WINAPI
--
-- NOTES:
--	Idle loop thread function
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI idleLoop(LPVOID n) {
	idleStruct * pIdleStruct = (idleStruct*)n;

	while (*pIdleStruct->connected)
	{
		DWORD returnValue = WaitForSingleObject(pIdleStruct->hInIdle, INFINITE);
		OutputDebugString("Finished waiting\n");

		switch (returnValue)
		{
		case WAIT_OBJECT_0:
			OutputDebugString("Sent EOT\n");
			sendFrame(*pIdleStruct, EOT_FRAME, CONTROL_FRAME_SIZE);
			std::this_thread::sleep_for(std::chrono::milliseconds(IDLE_EOT_PERIOD));
		case WAIT_TIMEOUT:
		default:
			break;
		}
	}
	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: createIdleLoop
--
-- DATE: November 24th, 2018
--
-- DESIGNER: Cameron Roberts, Simon Wu, Jacky Li, Phat Le
--
-- PROGRAMMER: Phat Le, Jacky Li
--
-- INTERFACE: void createIdleLoop(idleStruct& is)
--				1) idleStruct& is: struct that contains the idle trigger event and flag to indicate live connection.
--
-- RETURNS: void
--
-- NOTES:
--	Creates Idle loop thread function
----------------------------------------------------------------------------------------------------------------------*/
void createIdleLoop(idleStruct& is) {
	HANDLE hThrd;
	DWORD threadId;
	hThrd = CreateThread(NULL, 0, idleLoop, &is, 0, &threadId);
	if (hThrd)
	{
		CloseHandle(hThrd);
	}
}
