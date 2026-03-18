#pragma once
#include <string>
#include <vector>
#include "fs/FileSystem.h"

// Renderer owns all drawing logic.
// It never stores application state — it receives everything it needs
// as parameters so it stays a pure "view" layer.

namespace UI {

struct RenderState {
    std::string              currentPath;
    std::vector<FS::Entry>   entries;
    int                      cursorIndex;  // 0-based index into entries
    int                      scrollOffset; // first visible entry index
    std::string              statusMsg;    // shown in the status bar
    bool                     cmdLineActive  = false;
    std::string              cmdLineBuffer;
    bool                     searchActive   = false;
    std::string              searchQuery;   // current live-search query
    bool                     confirmActive  = false; // y/n prompt pending
    std::string              sortIndicator; // e.g. "name▲" shown in column header
    std::string              clipboard;     // path in clipboard (for paste display)
};

// Full redraw of the screen.
void draw(const RenderState& state);

// Redraw only the file list rows (cheaper than a full draw).
void drawFileList(const RenderState& state);

// Redraw only the status / command bar at the bottom.
void drawStatusBar(const RenderState& state);

} // namespace UI
