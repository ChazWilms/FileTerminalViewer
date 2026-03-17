#pragma once
#include <string>

// Abstract interface for all terminal/console operations.
// Platform-specific implementations live in ConsoleWindows.cpp (etc.)
// so that porting to Linux only requires a new ConsoleLinux.cpp.

namespace Platform {

struct Color {
    enum Code : int {
        Black       = 0,
        DarkBlue    = 1,
        DarkGreen   = 2,
        DarkCyan    = 3,
        DarkRed     = 4,
        DarkMagenta = 5,
        DarkYellow  = 6,
        Gray        = 7,
        DarkGray    = 8,
        Blue        = 9,
        Green       = 10,
        Cyan        = 11,
        Red         = 12,
        Magenta     = 13,
        Yellow      = 14,
        White       = 15,
        Default     = White,
    };
};

struct ConsoleDimensions {
    int width;
    int height;
};

// --- Lifecycle ---

// Initialize the console (enable raw input, hide cursor, etc.)
void init();

// Restore the console to its original state before exiting.
void shutdown();

// --- Output ---

// Clear the entire screen buffer.
void clearScreen();

// Move the cursor to (col, row), 0-indexed.
void setCursorPos(int col, int row);

// Show or hide the cursor.
void setCursorVisible(bool visible);

// Write a string at the current cursor position with the given
// foreground and background colors.
void writeText(const std::string& text,
               Color::Code fg = Color::White,
               Color::Code bg = Color::Black);

// Write a string at a specific position.
void writeTextAt(int col, int row, const std::string& text,
                 Color::Code fg = Color::White,
                 Color::Code bg = Color::Black);

// Fill an entire row with spaces (used to clear/redraw a line).
void clearLine(int row, Color::Code bg = Color::Black);

// --- Input ---

// Block until a key is pressed and return its virtual key code.
// On Windows this wraps _getch() / ReadConsoleInput.
int readKey();

// --- Dimensions ---

ConsoleDimensions getDimensions();

} // namespace Platform
