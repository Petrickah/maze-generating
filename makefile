all: ConsoleEngine.exe

ConsoleEngine.exe: main.cpp
	g++ -g -DWIN32 -DUNICODE -o ConsoleEngine.exe main.cpp