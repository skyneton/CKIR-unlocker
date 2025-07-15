#pragma once
#include <iostream>
#include <Windows.h>

inline std::ostream& red(std::ostream& s) {
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(out, FOREGROUND_RED | FOREGROUND_INTENSITY);
	return s;
}

inline std::ostream& green(std::ostream& s) {
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(out, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	return s;
}

inline std::ostream& blue(std::ostream& s) {
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(out, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	return s;
}

inline std::ostream& magenta(std::ostream& s) {
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(out, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	return s;
}

inline std::ostream& yellow(std::ostream& s) {
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(out, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	return s;
}

inline std::ostream& cyan(std::ostream& s) {
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(out, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	return s;
}

inline std::ostream& black(std::ostream& s) {
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(out, FOREGROUND_INTENSITY);
	return s;
}

inline std::ostream& white(std::ostream& s) {
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(out, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	return s;
}