/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: DataLink.cpp
--
-- PROGRAM: Final Project
--
-- FUNCTIONS:
-- void createMultipleDataFrames(ifstream& inFile, dataEventStruct*);
-- bool grabData(char* buffer, ifstream& inFile);
-- void addPadding(int index, char* buffer);
-- bool checkFrame(const char* frame, const int& rlen);
-- void createReadCommLoop(comInputStruct&);
-- void sendFrame(comInputStruct& comInput,const char* frame, size_t frameSize);
-- void sendFrame(idleStruct&, const char* frame, size_t frameSize);
-- unsigned int readFrame(comInputStruct&, unsigned int, int, long, char*, int&, HANDLE);
-- unsigned int maskFromFrame(char*, comInputStruct&);
--
-- DATE: December 5, 2008
--
-- REVISIONS:	December 5, 2018
--					-created function: checkFrame
--				December 2, 2018
--					-created function: sendFrame
--					-updated function: createMultipleDataFrames, readFrame, maskFromFrame
--				November 29, 2018
--					-updated function: readFrame
--				November 26, 2018
--					-updated function: createMultipleDataFrames, grabData, addPadding, createReadComLoop, sendFrame
--				November 25, 2018
--					-created function: readFrame, sendFrame, maskFromFrame
--				November 24, 2018
--					-created function: createMultipleDataFrames, grabData, addPadding, createReadComLoop
--				
-- DESIGNER: Phat Le, Cameron Roberts, Simon Wu, Jacky Li
--
-- PROGRAMMER: Phat Le, Cameron Roberts, Simon Wu, Jacky Li
--
-- NOTES:
-- The data link layer of the protocol. Handles the communication between the Session and the Physical layer. 
-- Packages the frame to be send. Conducts CRC checks on the data frame. Filters the received control frame (masking)
----------------------------------------------------------------------------------------------------------------------*/
#include "DataLink.h"

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: createMultipleDataFrames
--
-- DATE: December 2, 2018
--
-- REVISIONS:	December 2, 2018
--					-add CRC
--				November 26, 2018
--					-integration with main
--				November 24, 2018
--					-initial program
--
-- DESIGNER: Phat Le
--
-- PROGRAMMER: Phat Le
--
-- INTERFACE: createMultipleDataFrames(ifstream& inFile, dataEventStruct* pointerFileData)
--				1) ifstream& inFile: the input file we are extracting data from
--				2) dataEventStruct* pointerFileData: the pointer to the event struct that string vector to put
--				   data into
--
-- RETURNS: void
--
-- NOTES:
-- Frames the data taken from the input file and put them into the string vector in the dataEventStruct.
-- Add CRC check byte to frame.
----------------------------------------------------------------------------------------------------------------------*/
void createMultipleDataFrames(ifstream& inFile, dataEventStruct* pointerFileData) {
	bool finishedReading = false;
	for (int i = 0; !finishedReading; ++i) {
		
		char dataFrame[FRAME_SIZE];
		dataFrame[0] = SYN;
		
		dataFrame[1] = DC1;
		
		printNumber(i);
		finishedReading = grabData(dataFrame, inFile);
		
		// CRC function
		std::uint8_t crc8 = CRC::Calculate(dataFrame + 2, DATA_SIZE, CRC::CRC_8());
		dataFrame[FRAME_SIZE-1] = crc8;
		printStringNumber("createMult CRC: ", dataFrame[FRAME_SIZE - 1]);
		pointerFileData->data.emplace_back(dataFrame);
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: grabData
--
-- DATE: November 26, 2018
--
-- REVISIONS:	November 26, 2018
--					-integration with main
--				November 24, 2018
--					-initial program
--
-- DESIGNER: Phat Le
--
-- PROGRAMMER: Phat Le
--
-- INTERFACE: bool grabData(char* buffer, ifstream& inFile)
--				1) char* buffer: the character buffer being populated with the data
--				2) ifstream& inFile: the file we are reading the data from
--
-- RETURNS: bool
--				returns true if padding is added
--
-- NOTES:
-- Gets data from the input file.
-- Add padding of DC4 if its the last frame.
----------------------------------------------------------------------------------------------------------------------*/
bool grabData(char* buffer, ifstream& inFile) {
	char c;
	for (int i = 2; i < FRAME_SIZE-CRC_BYTES; ++i) {
		inFile.get(c);
		
		if (inFile.eof()) {
			addPadding(i, buffer);
			OutputDebugString("Reached EOF");
			return true;
		}
		buffer[i] = c;
	}
	return false;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: addPadding
--
-- DATE: November 26, 2018
--
-- REVISIONS:	November 26, 2018
--					-integration with main
--				November 24, 2018
--					-initial program
--
-- DESIGNER: Phat Le
--
-- PROGRAMMER: Phat Le
--
-- INTERFACE: void addPadding(int index, char* buffer)
--				1) int index: the index to start adding paddings (of DC4)
--				2) char* buffer: the character buffer being padded with DC4
--
-- RETURNS: void
--
-- NOTES:
-- Adds padding (of DC4) to the data buffer to be sent.
----------------------------------------------------------------------------------------------------------------------*/
void addPadding(int index, char* buffer) {
	
	buffer[index] = -1;

	for (int i = index + 1; i < FRAME_SIZE-CRC_BYTES; ++i) {
		buffer[i] = DC4;
	}

}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: checkFrame
--
-- DATE: December 5, 2018
--
-- DESIGNER: Jacky Li
--
-- PROGRAMMER: Jacky Li
--
-- INTERFACE: bool checkFrame(const char* frame, const int& rlen)
--				1) const char* frame: the character frame we are checking the CRC on
--				2) const int& rlen: the length of the character frame
--
-- RETURNS: bool
--				whether the CRC check succeeds or fails. Returns true for success.
--
-- NOTES:
-- Check CRC of the frame.
----------------------------------------------------------------------------------------------------------------------*/
bool checkFrame(const char* frame, const int& rlen) {
	std::uint8_t crc8 = CRC::Calculate(frame + 2, rlen - 2, CRC::CRC_8());
	return !crc8;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: createReadCommLoop
--
-- DATE: November 26, 2018
--
-- REVISIONS:	November 26, 2018
--					-integration with main
--				November 24, 2018
--					-initial program
--
-- DESIGNER: Cameron Roberts
--
-- PROGRAMMER: Cameron Roberts
--
-- INTERFACE: void createReadCommLoop(comInputStruct& comInput)
--				1) comInputStruct& comInput: struct that contains a vector of strings that holds the read frames,
--				   the event to trigger reading from COM port, and flag to indicate live connection. 
--
-- RETURNS: void
--
-- NOTES:
-- Taken from Assignment 1.
-- Creates the thread to read from COM port.
----------------------------------------------------------------------------------------------------------------------*/
void createReadCommLoop(comInputStruct& comInput) {
	HANDLE hThrd;
	DWORD threadId;
	hThrd = CreateThread(NULL, 0, readComLoop, &comInput, 0, &threadId);
	if (hThrd)
	{
		CloseHandle(hThrd);
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: sendFrame
--
-- DATE: November 26, 2018
--
-- REVISIONS:	November 26, 2018
--					-modified to work with sending frames
--				November 25, 2018
--					-initial program
--
-- DESIGNER: Cameron Roberts
--
-- PROGRAMMER: Cameron Roberts
--
-- INTERFACE: void sendFrame(comInputStruct& comInput, const char* frame, size_t frameSize)
--				1) comInputStruct& comInput: struct that contains a vector of strings that holds the read frames,
--				   the event that triggers when a frame is received, and flag to indicate live connection.
--				2) const char* frame: the frame being written to COM Port
--				3) size_t frameSize: the size of the frame
--
-- RETURNS: void
--
-- NOTES:
-- Taken from Assignment 1.
-- Calls Physical layer to send a frame to COM port.
----------------------------------------------------------------------------------------------------------------------*/
void sendFrame(comInputStruct& comInput, const char* frame, size_t frameSize) {
	writeComPort(comInput.fhand, frame, frameSize);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: sendFrame
--
-- DATE: December 2, 2018
--
-- DESIGNER: Cameron Roberts, Simon Wu
--
-- PROGRAMMER: Cameron Roberts
--
-- INTERFACE: sendFrame(idleStruct& is, const char* frame, size_t frameSize)
--				1) idleStruct& is: struct that contains the idle trigger event and flag to indicate live connection.
--				2) const char* frame: the frame being written to COM Port
--				3) size_t frameSize: the size of the frame
--
-- RETURNS: void
--
-- NOTES:
-- Same as the other sendFrame function, but overloaded to accept idleStruct instead. This version is called by
-- the idle loop.
----------------------------------------------------------------------------------------------------------------------*/
void sendFrame(idleStruct& is, const char* frame, size_t frameSize) {
	writeComPort(is.fhand, frame, frameSize);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: readFrame
--
-- DATE: December 2, 2018
--
-- REVISIONS:	December 2, 2018
--					-moved to DataLink layer
--				November 29, 2018
--					-edited timeout and maxRetries
--				November 25, 2018
--					-initial program
--
-- DESIGNER: Cameron Roberts, Simon Wu, Jacky Li, Phat Le
--
-- PROGRAMMER: Cameron Roberts
--
-- INTERFACE: readFrame(comInputStruct& comInput, unsigned int mask, int maxRetries, long timeout, char* rBuffer, 
--						int& rlen, HANDLE evnt)
--				1) comInputStruct& comInput: struct that contains a vector of strings that holds the read frames,
--				   the event that triggers when a frame is received, and flag to indicate live connection.
--				2) unsigned int mask: the control byte mask that we are filtering for
--				3) int maxRetries: the number of retries
--				4) long timeout: the length of the timeout
--				5) char* rBuffer: the buffer that holds the control frame
--				6) int& rlen: the length of the buffer
--				7) HANDLE evnt: the event to trigger when data is framed and ready
--
-- RETURNS: unsigned int
--
-- NOTES:
-- Attempt to read a frame and returns a mask corresponding to the type of frame received.
-- Will keep attempting until exceeds maxRetries.
-- If called by idle, will continously run (with no maxRetries limit)
----------------------------------------------------------------------------------------------------------------------*/
unsigned int readFrame(comInputStruct& comInput, unsigned int mask, int maxRetries, long timeout, char* rBuffer, int& rlen, HANDLE evnt) {
	OutputDebugString("readFrame started\n");
	auto currentTimeout = timeout;

	int retries = maxRetries;

	while (retries || maxRetries == -1)
	{
		currentTimeout = timeout;
		while (currentTimeout > 0) {
			int status = getFrameFromCom(comInput, rBuffer, rlen, currentTimeout, evnt);
			switch (status) {
			case 0:
				if (currentTimeout <= 0) {
					retries--;
				}
				break;
			case 1:
			{
				printStringNumber("rlen ", rlen);
				printStringNumber("rBuffer[0] ", rBuffer[0]);
				printStringNumber("rBuffer[1] ", rBuffer[1]);
				printStringNumber("rBuffer[2] ", rBuffer[2]);

				unsigned int temp_mask = maskFromFrame(rBuffer, comInput);

				printStringNumber("temp_mask = ", temp_mask);
				if ((temp_mask ^ mask) < mask) {
					return temp_mask;
				}
				break;
			}
			case 2:
				return EVENT_MASK;
			}
		}
	}
	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: maskFromFrame
--
-- DATE: December 2, 2018
--
-- REVISIONS:	December 2, 2018
--					-sets the event to update status of file transfer
--					-added code to update display on sending and receiving status
--				November 25, 2018
--					-initial program
--
-- DESIGNER: Cameron Roberts, Simon Wu, Jacky Li, Phat Le
--
-- PROGRAMMER: Simon Wu, Cameron Roberts
--
-- INTERFACE: unsigned int maskFromFrame(char* frame, comInputStruct& comInput)
--				1) char* frame: the frame taht we are checking for the mask
--				2) comInputStruct& comInput: struct that contains a vector of strings that holds the read frames,
--				   the event that triggers when a frame is received, and flag to indicate live connection.
--
-- RETURNS: unsigned int
--
-- NOTES:
-- Returns the corresponding mask for the frame (the mask determines what kind of frame is received)
----------------------------------------------------------------------------------------------------------------------*/
unsigned int maskFromFrame(char* frame, comInputStruct& comInput) {
	switch (frame[1])
	{
	case DC1:
		OutputDebugString("Mask evaluated as DATAFRAME\n");
		return DATA_FRAME_MASK;
	case DC2:
		OutputDebugString("Mask evaluated as DATAFRAME\n");
		return DATA_FRAME_MASK;
	case NAK:
		OutputDebugString("Mask evaluated as NAK\n");
		comInput.counter.nakCounter++;
		SetEvent(comInput.counter.statsEvent);
		return NAK_MASK;
	case ACK:
		OutputDebugString("Mask evaluated as ACK\n");
		comInput.counter.ackCounter++;
		SetEvent(comInput.counter.statsEvent);
		if (frame[2] == DC1) {
			return ACK0_MASK;
		}
		else if (frame[2] == DC2) {
			return ACK1_MASK;
		}
		return 0;
	case ENQ:
		OutputDebugString("Mask evaluated as ENQ\n");
		comInput.counter.enqCounter++;
		SetEvent(comInput.counter.statsEvent);
		return ENQ_MASK;
	case EOT:
		OutputDebugString("Mask evaluated as EOT\n");
		comInput.counter.eotCounter++;
		SetEvent(comInput.counter.statsEvent);
		return EOT_MASK;
	default:
		return 0;
	}
}
