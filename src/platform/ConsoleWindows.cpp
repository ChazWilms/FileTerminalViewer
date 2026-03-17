#ifdef PLATFORM_WINDOWS

#include "Console.h"
#include <windows.h>
#include <conio.h>
#include <stdexcept>

// Virtual key codes re-exported so callers don't need <windows.h>.
// These match the Windows VK_* constants used in readKey().
// Callers compare against Platform::Key::* constants defined in Console.h
// (added below as a convenience namespace).

namespace Platform {

// -----------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------

static HANDLE hOut = INVALID_HANDLE_VALUE;
static HANDLE hIn  = INVALID_HANDLE_VALUE;
static CONSOLE_SCREEN_BUFFER_INFO originalSBI = {};
static DWORD originalInMode  = 0;
static DWORD originalOutMode = 0;

static WORD makeAttr(Color::Code fg, Color::Code bg) {
    return static_cast<WORD>(fg) | (static_cast<WORD>(bg) << 4);
}

// -----------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------

void init() {
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    hIn  = GetStdHandle(STD_INPUT_HANDLE);

    if (hOut == INVALID_HANDLE_VALUE || hIn == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("TFV: Failed to get console handles.");
    }

    // Save original state so shutdown() can restore it.
    GetConsoleScreenBufferInfo(hOut, &originalSBI);
    GetConsoleMode(hIn,  &originalInMode);
    GetConsoleMode(hOut, &originalOutMode);

    // Enable virtual terminal processing for ANSI sequences (Win10+).
    DWORD outMode = originalOutMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, outMode);

    // Disable line input and echo so we get raw keypresses.
    DWORD inMode = originalInMode
        & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
    SetConsoleMode(hIn, inMode);

    setCursorVisible(false);
    clearScreen();
}

void shutdown() {
    setCursorVisible(true);
    // Restore modes.
    if (hIn  != INVALID_HANDLE_VALUE) SetConsoleMode(hIn,  originalInMode);
    if (hOut != INVALID_HANDLE_VALUE) SetConsoleMode(hOut, originalOutMode);
    // Restore original color.
    SetConsoleTextAttribute(hOut, originalSBI.wAttributes);
    clearScreen();
    setCursorPos(0, 0);
}

// -----------------------------------------------------------------------
// Output
// -----------------------------------------------------------------------

void clearScreen() {
    if (hOut == INVALID_HANDLE_VALUE) return;
    CONSOLE_SCREEN_BUFFER_INFO sbi;
    GetConsoleScreenBufferInfo(hOut, &sbi);
    DWORD size = sbi.dwSize.X * sbi.dwSize.Y;
    COORD origin = {0, 0};
    DWORD written;
    FillConsoleOutputCharacterA(hOut, ' ', size, origin, &written);
    FillConsoleOutputAttribute(hOut, sbi.wAttributes, size, origin, &written);
    SetConsoleCursorPosition(hOut, origin);
}

void setCursorPos(int col, int row) {
    COORD pos;
    pos.X = static_cast<SHORT>(col);
    pos.Y = static_cast<SHORT>(row);
    SetConsoleCursorPosition(hOut, pos);
}

void setCursorVisible(bool visible) {
    CONSOLE_CURSOR_INFO cci;
    GetConsoleCursorInfo(hOut, &cci);
    cci.bVisible = visible ? TRUE : FALSE;
    SetConsoleCursorInfo(hOut, &cci);
}

void writeText(const std::string& text, Color::Code fg, Color::Code bg) {
    SetConsoleTextAttribute(hOut, makeAttr(fg, bg));
    DWORD written;
    WriteConsoleA(hOut, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
}

void writeTextAt(int col, int row, const std::string& text,
                 Color::Code fg, Color::Code bg) {
    setCursorPos(col, row);
    writeText(text, fg, bg);
}

void clearLine(int row, Color::Code bg) {
    ConsoleDimensions dim = getDimensions();
    std::string blank(dim.width, ' ');
    writeTextAt(0, row, blank, Color::Black, bg);
}

// -----------------------------------------------------------------------
// Input
// -----------------------------------------------------------------------

// readKey() returns:
//   - Printable ASCII  -> the ASCII value directly (e.g. 'a' = 97)
//   - Special keys     -> 1000 + VK code  (e.g. VK_UP = 38 -> 1038)
//   - Extended prefix  -> two _getch() calls needed (0x00 or 0xE0 prefix)

int readKey() {
    int c = _getch();
    if (c == 0 || c == 0xE0) {
        // Extended / function key — read the actual key code.
        int ext = _getch();
        return 1000 + ext;
    }
    return c;
}

// -----------------------------------------------------------------------
// Dimensions
// -----------------------------------------------------------------------

ConsoleDimensions getDimensions() {
    CONSOLE_SCREEN_BUFFER_INFO sbi;
    if (GetConsoleScreenBufferInfo(hOut, &sbi)) {
        return {
            sbi.srWindow.Right  - sbi.srWindow.Left + 1,
            sbi.srWindow.Bottom - sbi.srWindow.Top  + 1
        };
    }
    return {80, 24}; // sensible fallback
}

} // namespace Platform

#endif // PLATFORM_WINDOWS
