#include "Renderer.h"
#include "platform/Console.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cctype>

namespace UI {

using PC = Platform::Color;

// -----------------------------------------------------------------------
// Layout constants
// -----------------------------------------------------------------------

static constexpr int HEADER_ROWS  = 2; // path + separator
static constexpr int FOOTER_ROWS  = 1; // status / command bar
static constexpr int COL_NAME_W   = 40;
static constexpr int COL_SIZE_W   = 12;
static constexpr int COL_DATE_W   = 18;

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------

static std::string truncate(const std::string& s, int maxLen) {
    if (static_cast<int>(s.size()) <= maxLen) return s;
    return s.substr(0, maxLen - 3) + "...";
}

static std::string formatSize(long long bytes) {
    if (bytes < 0) return "<DIR>";
    if (bytes < 1024)               return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024)        return std::to_string(bytes / 1024) + " KB";
    if (bytes < 1024 * 1024 * 1024) return std::to_string(bytes / (1024*1024)) + " MB";
    return std::to_string(bytes / (1024LL*1024*1024)) + " GB";
}

static std::string padRight(const std::string& s, int width) {
    if (static_cast<int>(s.size()) >= width) return s.substr(0, width);
    return s + std::string(width - s.size(), ' ');
}

static std::string padLeft(const std::string& s, int width) {
    if (static_cast<int>(s.size()) >= width) return s.substr(0, width);
    return std::string(width - s.size(), ' ') + s;
}

// Case-insensitive substring search. Returns npos if not found.
static size_t findCI(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return std::string::npos;
    std::string lh = haystack, ln = needle;
    std::transform(lh.begin(), lh.end(), lh.begin(), ::tolower);
    std::transform(ln.begin(), ln.end(), ln.begin(), ::tolower);
    return lh.find(ln);
}

// -----------------------------------------------------------------------
// Header
// -----------------------------------------------------------------------

static void drawHeader(const RenderState& state) {
    auto dim = Platform::getDimensions();

    // Row 0: current path (tinted differently during search).
    PC::Code headerBg = state.searchActive ? PC::DarkMagenta : PC::DarkBlue;
    Platform::clearLine(0, headerBg);
    std::string pathLine = state.searchActive
        ? " TFV [SEARCH] | " + state.currentPath
        : " TFV | " + state.currentPath;
    pathLine = truncate(pathLine, dim.width);
    Platform::writeTextAt(0, 0, padRight(pathLine, dim.width), PC::White, headerBg);

    // Row 1: column headers.
    Platform::clearLine(1, PC::Black);
    std::string header =
        padRight(" Name", COL_NAME_W) +
        padLeft("Size",   COL_SIZE_W) + "  " +
        padRight("Modified", COL_DATE_W);
    Platform::writeTextAt(0, 1, truncate(header, dim.width), PC::DarkGray, PC::Black);
}

// -----------------------------------------------------------------------
// File list
// -----------------------------------------------------------------------

// Draw a single file-list row, optionally highlighting the search match
// within the name portion.
static void drawEntryRow(int screenRow, const FS::Entry& e,
                         bool isSelected, const std::string& searchQuery) {
    auto dim = Platform::getDimensions();

    PC::Code fg, bg;
    if (isSelected) {
        fg = PC::Black; bg = PC::Cyan;
    } else if (e.type == FS::EntryType::Directory) {
        fg = PC::Yellow; bg = PC::Black;
    } else if (e.type == FS::EntryType::Symlink) {
        fg = PC::Cyan; bg = PC::Black;
    } else {
        fg = PC::White; bg = PC::Black;
    }

    std::string prefix   = (e.type == FS::EntryType::Directory) ? "[D] " : "    ";
    std::string fullName = truncate(prefix + e.name, COL_NAME_W);
    std::string namePad  = padRight(fullName, COL_NAME_W);
    std::string sizePart = padLeft(formatSize(e.size), COL_SIZE_W);
    std::string datePart = padRight(e.modified, COL_DATE_W);
    std::string tail     = sizePart + "  " + datePart;

    // If not searching, write the whole row in one shot.
    if (searchQuery.empty() || isSelected) {
        std::string line = padRight(namePad + tail, dim.width);
        Platform::writeTextAt(0, screenRow, line, fg, bg);
        return;
    }

    // When searching, split the name column into three segments:
    // [before match] [match — highlighted] [after match]
    // Then append the tail columns normally.
    size_t matchPos = findCI(fullName, searchQuery);

    if (matchPos == std::string::npos) {
        // No match in display string — just draw normally.
        std::string line = padRight(namePad + tail, dim.width);
        Platform::writeTextAt(0, screenRow, line, fg, bg);
        return;
    }

    size_t matchEnd = matchPos + searchQuery.size();
    if (matchEnd > fullName.size()) matchEnd = fullName.size();

    std::string before = padRight(fullName.substr(0, matchPos), 0);
    std::string match  = fullName.substr(matchPos, matchEnd - matchPos);
    std::string after  = padRight(fullName.substr(matchEnd), COL_NAME_W - static_cast<int>(matchEnd));

    // Write the three name segments then the tail.
    Platform::writeTextAt(0, screenRow, before, fg, bg);
    int matchCol = static_cast<int>(matchPos);
    Platform::writeTextAt(matchCol, screenRow, match, PC::Black, PC::Yellow);
    int afterCol = static_cast<int>(matchEnd);
    Platform::writeTextAt(afterCol, screenRow, after, fg, bg);
    Platform::writeTextAt(COL_NAME_W, screenRow,
                          padRight(tail, dim.width - COL_NAME_W), fg, bg);
}

void drawFileList(const RenderState& state) {
    auto dim = Platform::getDimensions();
    int listRows = dim.height - HEADER_ROWS - FOOTER_ROWS;

    for (int row = 0; row < listRows; ++row) {
        int entryIdx  = state.scrollOffset + row;
        int screenRow = HEADER_ROWS + row;
        Platform::clearLine(screenRow, PC::Black);

        if (entryIdx >= static_cast<int>(state.entries.size())) continue;

        const auto& e = state.entries[entryIdx];
        bool isSelected = (entryIdx == state.cursorIndex);
        drawEntryRow(screenRow, e, isSelected,
                     state.searchActive ? state.searchQuery : "");
    }
}

// -----------------------------------------------------------------------
// Status / command bar
// -----------------------------------------------------------------------

void drawStatusBar(const RenderState& state) {
    auto dim = Platform::getDimensions();
    int row = dim.height - 1;
    Platform::clearLine(row, PC::DarkBlue);

    std::string text;
    bool showCursor = false;

    if (state.cmdLineActive) {
        text = ":" + state.cmdLineBuffer;
        showCursor = true;
    } else if (state.searchActive) {
        text = " / " + state.searchQuery;
        showCursor = true;
    } else {
        text = " " + state.statusMsg;
        if (!state.clipboard.empty()) {
            text += "  [clip: " + state.clipboard + "]";
        }
    }

    text = truncate(text, dim.width);
    PC::Code fg = (state.cmdLineActive || state.searchActive) ? PC::White : PC::Gray;
    Platform::writeTextAt(0, row, padRight(text, dim.width), fg, PC::DarkBlue);

    if (showCursor) {
        Platform::setCursorPos(static_cast<int>(text.size()), row);
        Platform::setCursorVisible(true);
    } else {
        Platform::setCursorVisible(false);
    }
}

// -----------------------------------------------------------------------
// Full draw
// -----------------------------------------------------------------------

void draw(const RenderState& state) {
    drawHeader(state);
    drawFileList(state);
    drawStatusBar(state);
}

} // namespace UI
