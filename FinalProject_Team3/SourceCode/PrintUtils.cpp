#include "PrintUtils.h"

void printNumber(int n) {
	char s[2056];
	sprintf_s(s, "%d\n", n);
	OutputDebugString(s);
}

void printChar(char c) {
	char s[2056];
	sprintf_s(s, "%c\n", c);
	OutputDebugString(s);
}

void printStringNumber(const char * s, int n) {
	OutputDebugString(s);
	printNumber(n);
}

void printCharArray(std::string s, size_t size) {
	OutputDebugString(s.c_str());
}

void printStringChar(const char * s, char c) {
	OutputDebugString(s);
	printChar(c);
}