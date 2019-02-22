#if !defined(_evoConsoleGameEngine_H)
#define _evoConsoleGameEngine_H

#include <Windows.h>
#include <stdio.h>

typedef struct _CONSOLE_FONT_INFOEX {
    ULONG cbSize;
    DWORD nFont;
    COORD dwFontSize;
    UINT  FontFamily;
    UINT  FontWeight;
    WCHAR FaceName[LF_FACESIZE];
} CONSOLE_FONT_INFOEX, *PCONSOLE_FONT_INFOEX;
#ifdef __cplusplus
extern "C" {
#endif
BOOL WINAPI SetCurrentConsoleFontEx(HANDLE hConsoleOutput, BOOL bMaximumWindow, PCONSOLE_FONT_INFOEX lpConsoleCurrentFontEx);
#ifdef __cplusplus
}
#endif

enum COLOUR {
	FG_BLACK		= 0x0000, BG_BLACK		    = 0x0000,
	FG_DARK_BLUE    = 0x0001, BG_DARK_BLUE	    = 0x0010,
	FG_DARK_GREEN   = 0x0002, BG_DARK_GREEN	    = 0x0020,
	FG_DARK_CYAN    = 0x0003, BG_DARK_CYAN	    = 0x0030,
	FG_DARK_RED     = 0x0004, BG_DARK_RED	    = 0x0040,
	FG_DARK_MAGENTA = 0x0005, BG_DARK_MAGENTA   = 0x0050,
	FG_DARK_YELLOW  = 0x0006, BG_DARK_YELLOW	= 0x0060,
	FG_GREY			= 0x0007, BG_GREY			= 0x0070,
	FG_DARK_GREY    = 0x0008, BG_DARK_GREY	    = 0x0080,
	FG_BLUE			= 0x0009, BG_BLUE			= 0x0090,
	FG_GREEN		= 0x000A, BG_GREEN		    = 0x00A0,
	FG_CYAN			= 0x000B, BG_CYAN			= 0x00B0,
	FG_RED			= 0x000C, BG_RED			= 0x00C0,
	FG_MAGENTA		= 0x000D, BG_MAGENTA		= 0x00D0,
	FG_YELLOW		= 0x000E, BG_YELLOW		    = 0x00E0,
	FG_WHITE		= 0x000F, BG_WHITE		    = 0x00F0,
};

enum PIXEL_TYPE {
	PIXEL_SOLID = 0x2588,
	PIXEL_THREEQUARTERS = 0x2593,
	PIXEL_HALF = 0x2592,
	PIXEL_QUARTER = 0x2591,
};

template<typename T>
struct Size { T Width, Height; };
struct StandardHandle { HANDLE in, out, console; };

class ConsoleGameEngine {
    StandardHandle hStd;
    DWORD dwCharactersWritten;
    SMALL_RECT rectWindow;
    CHAR_INFO *screen;
    bool running;
protected:
    Size<short> console;
    virtual bool onInitializationRun() = 0;
    virtual bool onUpdateCallback() = 0;
    virtual bool onKeyboardExit() = 0;
public:
    virtual ~ConsoleGameEngine() {
        delete[] screen;
    }
    int CreateConsole(Size<short> size, short pWidth, short pHeight) {
        this->console = size;
        // Create Console Handlers
        this->hStd = {
            GetStdHandle(STD_OUTPUT_HANDLE), 
            GetStdHandle(STD_INPUT_HANDLE),
            CreateConsoleScreenBuffer(
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                CONSOLE_TEXTMODE_BUFFER,
                NULL
            )
        };
        auto DisplayError = [](const bool cond, const wchar_t* wstr_format, const int value = 1){
            if(cond) {
                wprintf(L"%s - (%d)\n", wstr_format, value);
                throw value;
            }
        };
        try {
            // Set Console Size
            this->rectWindow = { 0, 0, (SHORT)(this->console.Width - 1), (SHORT)(this->console.Height - 1) };
            DisplayError(SetConsoleWindowInfo(this->hStd.console, TRUE, &this->rectWindow), L"Set Console Window Info failed", GetLastError());
            DisplayError(this->hStd.out == INVALID_HANDLE_VALUE || this->hStd.console == INVALID_HANDLE_VALUE, L"Create Console Buffer failed");
            // Set Console Screen Buffer
            DisplayError(!SetConsoleScreenBufferSize(this->hStd.console, {this->console.Width, this->console.Height}), L"Set Console Screen Size", GetLastError());
            DisplayError(!SetConsoleActiveScreenBuffer(hStd.console), L"Set Console Buffer failed", GetLastError());
            // Set Console Font Details
            CONSOLE_FONT_INFOEX cfi = {
                sizeof(CONSOLE_FONT_INFOEX), 0, pWidth, pHeight, FF_DONTCARE, FW_NORMAL, L"Consolas"
            };
            DisplayError(!SetCurrentConsoleFontEx(this->hStd.console, false, &cfi), L"Set Current Font failed", GetLastError());
            // Get Console Screen Buffer Info
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            DisplayError(!GetConsoleScreenBufferInfo(this->hStd.console, &csbi), L"Get Console Screen Buffer Info failed", GetLastError());
            DisplayError(this->console.Height > csbi.dwMaximumWindowSize.Y, L"Screen Height / Font Height Too Big");
            DisplayError(this->console.Width > csbi.dwMaximumWindowSize.X, L"Screen Width / Font Width Too Big");
            // Set Physical Console Window Size
            DisplayError(!SetConsoleWindowInfo(this->hStd.console, TRUE, &this->rectWindow), L"Set Console Window Info failed", GetLastError());
            // Set flags to allow mouse input		
            DisplayError(!SetConsoleMode(this->hStd.out, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT), L"Set Console Mode failed", GetLastError());
        } catch(int v) {
            return v;
        }

		// Allocate memory for screen buffer
		this->screen = new CHAR_INFO[this->console.Width*this->console.Height];
		memset(this->screen, 0, sizeof(wchar_t) * this->console.Width * this->console.Height);

        return 0;
    }
    virtual void Draw(int x, int y, short c = 0x2588, short col = 0x000F) {
		if (x >= 0 && x < this->console.Width && y >= 0 && y < this->console.Height)
		{
			this->screen[y * this->console.Width + x].Char.UnicodeChar = c;
			this->screen[y * this->console.Width + x].Attributes = col;
		}
	}
    virtual void DrawRectangle(int x0, int y0, int x1, int y1, PIXEL_TYPE glyph = PIXEL_SOLID, short int colour = FG_BLACK) {
        for(int i=0; i<x1; i++) {
            for(int j=0; j<y1; j++) {
                Draw(x0+i, y0+j, glyph, colour);
            }
        }
    }
    void Start() {
        this->running = true;
        this->onInitializationRun();
        do {
            this->onUpdateCallback();
            if(this->onKeyboardExit()) {
                this->running = false;
            }
            WriteConsoleOutput(this->hStd.console, this->screen, {this->console.Width, this->console.Height}, {0, 0}, &this->rectWindow); 
        }while(this->running);
    }
};

#endif // _evoConsoleGameEngine_H
