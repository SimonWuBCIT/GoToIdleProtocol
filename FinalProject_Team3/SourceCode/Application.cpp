/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: Application.cpp
--
-- PROGRAM: Final Project
--
-- FUNCTIONS:
-- int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hprevInstance, LPSTR lspszCmdParam, int nCmdShow)
-- LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
-- std::vector<int> findValidComPorts();
-- void fillConnectionMenu();
-- void initOpenFileName();
-- DWORD WINAPI saveFileLoop(LPVOID n);
-- DWORD WINAPI statisticThread(LPVOID n);
-- std::ofstream prepareWrite();
-- void printToFile(std::ofstream& out_file, const char* buffer, size_t buffer_size);
-- void enableMenu();
--
-- DATE: December 5, 2018
--
-- REVISIONS:	December 5, 2018
--					-created function: statisticThread, enableMenu
--					-updated function: WndProc
--				November 30, 2018
--					-updated function: saveFileLoop
--				November 29, 2018
--					-created function: saveFileLoop
--					-updated function: prepareWrite, printToFile
--				November 25, 2018
--					-created function: initOpenFileName
--					-updated function: WndProc
--				November 24, 2018
--					-created function: prepareWrite, printToFile
--				September 30, 2018
--					-taken from previous Assignment 1: WinMain, WndProc, printToScreen, findValidComPorts, 
--					fillConnectionMenu, 
--
-- DESIGNER: Phat Le, Cameron Roberts, Simon Wu, Jacky Li
--
-- PROGRAMMER: Phat Le, Cameron Roberts, Simon Wu, Jacky Li
--
-- NOTES:
-- Handles the UI (contains the menu for connect, selecting COM port, selecting COM port settings, and painting file
-- sending/receiving status to the window. Calls the Session class to setup the COM port
----------------------------------------------------------------------------------------------------------------------*/
#define STRICT

#include "Application.h"

protocolStruct ps;

char Name[] = "Final Project";
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

COMMCONFIG	cc;
HANDLE hComm;
UINT_PTR conMenu;
std::ofstream out_file;

HWND hwnd;

// Currently opened file STRUCT
OPENFILENAME ofn;
HANDLE ofh;             // file handle
char szFile[260];       // buffer for file name

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: WinMain
--
-- DATE: September 30, 2018
--
-- DESIGNER: Cameron Roberts
--
-- PROGRAMMER: Cameron Roberts
--
-- INTERFACE: int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hprevInstance, LPSTR lspszCmdParam, int nCmdShow)
--
-- RETURNS: int WINAPI
--
-- NOTES:
-- Taken from Assignment 1
-- Windows start of excecution function.
----------------------------------------------------------------------------------------------------------------------*/
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hprevInstance,
	LPSTR lspszCmdParam, int nCmdShow)
{
	MSG Msg;
	WNDCLASSEX Wcl;

	Wcl.cbSize = sizeof(WNDCLASSEX);
	Wcl.style = CS_HREDRAW | CS_VREDRAW;
	Wcl.hIcon = LoadIcon(NULL, IDI_APPLICATION); 
	Wcl.hIconSm = NULL; 
	Wcl.hCursor = LoadCursor(NULL, IDC_ARROW);

	Wcl.lpfnWndProc = WndProc;
	Wcl.hInstance = hInst;
	Wcl.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	Wcl.lpszClassName = Name;

	Wcl.lpszMenuName = "COMMENU"; 
	Wcl.cbClsExtra = 0;
	Wcl.cbWndExtra = 0;

	if (!RegisterClassEx(&Wcl))
		return 0;

	hwnd = CreateWindow(Name, Name, WS_OVERLAPPEDWINDOW, 10, 10,
		600, 400, NULL, NULL, hInst, NULL);

	fillConnectionMenu();

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	while (GetMessage(&Msg, NULL, 0, 0))
	{
		
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return Msg.wParam;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: WndProc
--
-- DATE: December 5, 2018
--
-- REVISIONS:	December 5, 2018
--					-Added WM_PAINT switch case to print status of file sending and receiving
--				November 25, 2018
--					-Added called to frame file for outputs
--				September 30, 2018
--					-initial program (taken from previous Assignment 1)
--
-- DESIGNER: Simon Wu, Cameron Roberts
--
-- PROGRAMMER: Simon Wu, Cameron Roberts
--
-- INTERFACE: LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
--
-- RETURNS: LRESULT CALLBACK
--
-- NOTES:
-- Function to handle Windows messages.
----------------------------------------------------------------------------------------------------------------------*/
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message,
	WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT paintstruct;

	switch (Message)
	{
	case WM_CHAR:	// Process keystroke
		if (wParam == VK_ESCAPE) {
			closeSession();
			EnableMenuItem(GetMenu(hwnd), conMenu, MF_ENABLED);
			DrawMenuBar(hwnd);
		}
		break;
	case WM_DESTROY:	
		PostQuitMessage(0);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_HELP:
			MessageBox(hwnd, TEXT("."), TEXT("Help"), MB_OK);
			break;
		case IDM_UPLOAD:
			// Pop up Dialog, get filepath back
			initOpenFileName();
			if (GetOpenFileName(&ofn) == TRUE) {
				frameFile(ofn.lpstrFile);
			}
			break;
		case IDM_EXIT:
			exit(0);
			break;
		}
		if (LOWORD(wParam) > IDM_PORT) {
			char comPort[COM_NAME_BUFFER];
			int portNumber = int(wParam) - IDM_PORT;
			sprintf_s(comPort, "COM%d", portNumber);

			cc.dwSize = sizeof(COMMCONFIG);
			cc.wVersion = 0x100;
			GetCommConfig(hComm, &cc, &cc.dwSize);
			if (!CommConfigDialog(comPort, hwnd, &cc)) {
				break;
			}

			HANDLE hThrd;
			DWORD threadId;

			if (!createSession(cc, comPort, ps)) {
				MessageBox(hwnd, TEXT("Failed to create connection"), TEXT("ERROR"), MB_OK);
			}
			else {
				EnableMenuItem(GetMenu(hwnd), conMenu, MF_DISABLED);
				DrawMenuBar(hwnd);
			}

			hThrd = CreateThread(NULL, 0, saveFileLoop, &(ps.fileData), 0, &threadId);
			if (hThrd)
			{
				CloseHandle(hThrd);
			}

			HANDLE counterThrd;
			DWORD counterThrdId;

			counterThrd = CreateThread(NULL, 0, statisticThread, &(ps.comInput.counter), 0, &counterThrdId);
			if (counterThrd) {
				CloseHandle(counterThrd);
			}
		}
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &paintstruct);
		{
			int y_position = 0;

			char message[256];
			sprintf_s(message, "Data frames sent: %d", ps.comInput.counter.sentDataFrameCounter);
			TextOut(hdc, 0, y_position += 15, message, strlen(message));

			sprintf_s(message, "Data frames left to sent: %d", ps.frameData.data.size());
			TextOut(hdc, 0, y_position += 15, message, strlen(message));

			sprintf_s(message, "Number of resends: %d", ps.comInput.counter.dataFrameResendCounter);
			TextOut(hdc, 0, y_position += 15, message, strlen(message));

			sprintf_s(message, "ACK received: %d", ps.comInput.counter.ackCounter);
			TextOut(hdc, 0, y_position += 15, message, strlen(message));

			sprintf_s(message, "NAK received: %d", ps.comInput.counter.nakCounter);
			TextOut(hdc, 0, y_position += 15, message, strlen(message));

			sprintf_s(message, "Good data frames received: %d", ps.comInput.counter.receivedGoodDataFrameCounter);
			TextOut(hdc, 0, y_position += 15, message, strlen(message));

			sprintf_s(message, "Bad data frames received: %d", ps.comInput.counter.receivedBadDataFrameCounter);
			TextOut(hdc, 0, y_position += 15, message, strlen(message));

			double good = ps.comInput.counter.receivedGoodDataFrameCounter;
			double bad = ps.comInput.counter.receivedBadDataFrameCounter;
			sprintf_s(message, "ERR RATE: %3.2f", (bad + good) == 0 ? 0 : bad/(bad + good) * 100);
			TextOut(hdc, 0, y_position += 15, message, strlen(message));

			sprintf_s(message, "ENQ received: %d", ps.comInput.counter.enqCounter);
			TextOut(hdc, 0, y_position += 15, message, strlen(message));

			sprintf_s(message, "EOT received: %d", ps.comInput.counter.eotCounter);
			TextOut(hdc, 0, y_position += 15, message, strlen(message));			
		}
		EndPaint(hwnd, &paintstruct);

		break;
	default:
		return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: printToScreen
--
-- DATE: September 30, 2018
--
-- DESIGNER: Cameron Roberts
--
-- PROGRAMMER: Cameron Roberts
--
-- INTERFACE: void printToScreen (const char* str)
--				1) const char* str: The string to be printed to the screen
--
-- RETURNS: void
--
-- NOTES:
-- Taken from Assignment 1.
-- This function will take the given string and print it to the screen using the TextOut function.
----------------------------------------------------------------------------------------------------------------------*/
void printToScreen(const char* str) {
	static unsigned charPosition = 0;

	HDC hdc;
	hdc = GetDC(hwnd);

	TextOut(hdc, CHARACTER_WIDTH * charPosition, 0, str, strlen(str));
	charPosition++;

	ReleaseDC(hwnd, hdc);
}


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: findValidComPorts
--
-- DATE: September 30, 2018
--
-- DESIGNER: Cameron Roberts
--
-- PROGRAMMER: Cameron Roberts
--
-- INTERFACE: std::vector<int> findValidComPorts() 
--
-- RETURNS: A vector containing the numbers of existing COM ports
--
-- NOTES:
-- Taken from Assignment 1.
-- This function will find a list of existing COM ports by attempting to call CreateFile on them
-- and will return a vector containing the number of the ones that exist.
----------------------------------------------------------------------------------------------------------------------*/
std::vector<int> findValidComPorts() {
	std::vector<int> validPorts;
	HANDLE fhndl;
	char comPort[COM_NAME_BUFFER];
	

	for (int i = 1; i <= COM_PORT_MAX; ++i) {
		if (i < 10) {
			sprintf_s(comPort, "COM%d", i); 
		}
		else {
			sprintf_s(comPort, "\\\\.\\COM%d", i); // different format needed for port numbers larget than 10
		}
		fhndl = CreateFile(comPort, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (fhndl == INVALID_HANDLE_VALUE) { // error opening port
			if (ERROR_ACCESS_DENIED == GetLastError() ) { // exists but is in use
				validPorts.push_back(i);
			}
			else {
			}
		}
		else { // port exists
			validPorts.push_back(i);
			CloseHandle(fhndl);
		}
	}
	return validPorts;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: fillConnectionMenu
--
-- DATE: September 30, 2018
--
-- DESIGNER: Cameron Roberts
--
-- PROGRAMMER: Cameron Roberts
--
-- INTERFACE: void fillConnectionMenu(void)
--
-- RETURNS: void
--
-- NOTES:
-- Taken from Assignment 1.
-- Creates a popup menu called Connect and a sub menuitem for each COM port
----------------------------------------------------------------------------------------------------------------------*/
void fillConnectionMenu() {

	HMENU hSubMenu = CreatePopupMenu();	
	conMenu = (UINT_PTR)hSubMenu;
	InsertMenu(GetMenu(hwnd), 0, MF_BYPOSITION| MF_STRING | MF_POPUP, conMenu , "&Connect");

	std::vector<int> ports = findValidComPorts();
	for (int i = ports.size()-1; i >= 0; --i) {
		char comPort[COM_NAME_BUFFER];
		int portNumber = ports.at(i);
		sprintf_s(comPort, "COM%d", portNumber);

		MENUITEMINFO menuInfo;
		menuInfo.fMask = MIIM_STRING | MIIM_ID;
		menuInfo.fType = MFT_STRING;
		menuInfo.fState = MFS_ENABLED;
		menuInfo.wID = IDM_PORT+portNumber;
		menuInfo.hSubMenu = NULL;
		menuInfo.hbmpChecked = NULL;
		menuInfo.hbmpUnchecked = NULL;
		menuInfo.dwTypeData = (LPTSTR)comPort;
		menuInfo.cch = 7;
		menuInfo.cbSize = sizeof(menuInfo);

		InsertMenuItem(GetSubMenu(GetMenu(hwnd), 0), 0, true, &menuInfo);
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: initOpenFileName
--
-- DATE: November 25, 2018
--
-- DESIGNER: Jacky Li
--
-- PROGRAMMER: Jacky Li
--
-- INTERFACE: void initOpenFileName(void)
--
-- RETURNS: void.
--
-- NOTES:
-- Inits memory and sets up the dialog box structure for uploading a file
----------------------------------------------------------------------------------------------------------------------*/
void initOpenFileName() {
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: prepareWrite
--
-- DATE: November 29, 2018
--
-- REVISIONS:	November 29, 2018
--					-modified check on whether a file exists (to prevent overriding existing files in the folder)
--					-integration with main code.
--				November 24, 2018
--					-initial program
--
-- DESIGNER: Simon Wu
--
-- PROGRAMMER: Simon Wu
--
-- INTERFACE: std::ofstream prepareWrite()
--
-- RETURNS: ofstream
--				-The file stream to write from com port to file
--
-- NOTES:
-- Init the file and open stream in preparation to write data
----------------------------------------------------------------------------------------------------------------------*/
std::ofstream prepareWrite()
{
	std::ofstream out_file;
	std::ifstream infile;
	std::string file_name;
	static int file_counter = 0;

	struct stat st;
	do
	{
		file_counter++;
		file_name = "file" + std::to_string(file_counter) + ".txt";
	} while (stat(file_name.c_str(), &st) == 0);
	out_file.open(file_name);
	return out_file;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: printToFile
--
-- DATE: November 29, 2018
--
-- REVISIONS:	November 29, 2018
--					-integration with main code.
--				November 24, 2018
--					-initial program
--
-- DESIGNER: Simon Wu
--
-- PROGRAMMER: Simon Wu
--
-- INTERFACE: void printToFile(std::ofstream& out_file ,const char* buffer, size_t buffer_size)
--				1) std::ofstream& out_file: the file being printed to (the output file)
--				2) const char* buffer: the buffer that contains the character we are printing to the file
--				3) size_t buffer_size: the size of the buffer
--
-- RETURNS: void
--
-- NOTES:
-- Write data (taken from the char buffer) from COM port to file.
-- Checks for EOF character to determine when file should close.
----------------------------------------------------------------------------------------------------------------------*/
void printToFile(std::ofstream& out_file ,const char* buffer, size_t buffer_size)
{

	if (!out_file.is_open()) {
		out_file = prepareWrite();
	}
	size_t counter = 0;
	while (counter < buffer_size)
	{
		if (buffer[counter] == EOF)
		{
			out_file.close();
			return;
		}
		//put character into the file
		if (buffer[counter] == '\r') {
			out_file << buffer[counter];
			out_file << '\n';
		}
		else
		{
			out_file << buffer[counter];
		}
		++counter;
	}
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: saveFileLoop
--
-- DATE: November 30, 2018
--
-- REVISIONS:	November 30, 2018
--					-modified program to read the dataEventStruct that contains the relevant events and variables
--					-modified program to handle interrupted file transferred (if other user disconnects)
--				November 29, 2018
--					-initial program
--
-- DESIGNER: Cameron Roberts, Simon Wu
--
-- PROGRAMMER: Cameron Roberts, Simon Wu
--
-- INTERFACE: DWORD WINAPI saveFileLoop(LPVOID n)
--				1) LPVOID n: cast to dataEventStruct*, the pointer to event struct that contains the data received
--				   (as strings), the event to trigger writing to file, and the flag to indicate live connection
--
-- RETURNS: DWORD
--
-- NOTES:
-- Triggers writing data to file.
-- Waits for the event (gotten via dataEventStruct) to trigger writing.
-- Prints file transfer error to the file if the the transfer is interrupted (complete disconnect).
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI saveFileLoop(LPVOID n) {
	dataEventStruct* fileData = (dataEventStruct*)n;

	OutputDebugString("START save file thread");
	while (*fileData->connected) {
		OutputDebugString("waiting for go ahead to print\n");

		WaitForSingleObject(fileData->hEvent, CONNECTION_TIMEOUT);
		OutputDebugString("ABOUT TO PRINT STUFF\n");

		ResetEvent(fileData->hEvent);

		printStringNumber("amount of things im about to print: ",fileData->data.size());
		while (fileData->data.size() != 0) {
			OutputDebugString("PRINTERD SOMETHING\n");
			printToFile(out_file, fileData->data[0].c_str(), fileData->data[0].size());
			fileData->data.erase(fileData->data.begin());
		}
	}
	if (out_file.is_open()) {
		std::string error = "<<<FILE TRANSFER INTERRUPTED>>";
		printToFile(out_file, error.c_str(), error.size());
		char eof = EOF;
		printToFile(out_file, &eof, 1);
		out_file.close();
	}

	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: statisticThread
--
-- DATE: December 5, 2018
--
-- DESIGNER: Simon Wu, Cameron Roberts
--
-- PROGRAMMER: Simon Wu
--
-- INTERFACE: DWORD WINAPI statisticThread(LPVOID n)
--				1) LPVOID n: cast to counterStruct*, the pointer to the struct that tracks the count of the sending
--				   and receving status
--
-- RETURNS: DWORD
--
-- NOTES:
-- Wait for event from counterStruct to trigger updating the window on file sending and receiving status.
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI statisticThread(LPVOID n) {
	counterStruct* counter = (counterStruct*) n;

	while (*counter->connected) {
		WaitForSingleObject(counter->statsEvent, CONNECTION_TIMEOUT);
		ResetEvent(counter->statsEvent);
		
		//print to window
		InvalidateRect(hwnd, NULL, true);
	}

	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: enableMenu
--
-- DATE: December 5, 2018
--
-- DESIGNER: Cameron Roberts
--
-- PROGRAMMER: Cameron Roberts
--
-- INTERFACE: DWORD WINAPI statisticThread(LPVOID n)
--				1) LPVOID n: cast to counterStruct*, the pointer to the struct that tracks the sending
--				   and receving status counters
--
-- RETURNS: DWORD
--
-- NOTES:
-- Enables the connect menu item
----------------------------------------------------------------------------------------------------------------------*/
void enableMenu() {
	EnableMenuItem(GetMenu(hwnd), conMenu, MF_ENABLED);
	DrawMenuBar(hwnd);
}