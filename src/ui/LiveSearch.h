#pragma once
#include "fs/FileSystem.h"
#include <string>
#include <vector>

// LiveSearch filters the current directory listing as the user types.
// It is purely stateful — the App owns one instance and feeds keys to it.
// The filtered view is retrieved via matches() and passed to the Renderer.

namespace UI {

class LiveSearch {
public:
    // Feed a keycode from Platform::readKey().
    // Returns true while search is still active, false when done (Enter/Escape).
    bool handleKey(int key);

    // Activate search mode and set the full entry list to filter against.
    void activate(const std::vector<FS::Entry>& allEntries);

    // Cancel without confirming.
    void cancel();

    // True while search mode is active.
    bool isActive() const { return m_active; }

    // Current query string (shown in the status bar).
    const std::string& query() const { return m_query; }

    // Filtered subset of the entries — updated on every keystroke.
    const std::vector<FS::Entry>& matches() const { return m_matches; }

    // Index of the currently highlighted match (0-based within matches()).
    int selectedIndex() const { return m_selectedIndex; }

    // True if the user pressed Enter to confirm (vs Escape to cancel).
    bool confirmed() const { return m_confirmed; }

private:
    bool                   m_active        = false;
    bool                   m_confirmed     = false;
    int                    m_selectedIndex = 0;
    std::string            m_query;
    std::vector<FS::Entry> m_allEntries;
    std::vector<FS::Entry> m_matches;

    void refilter();
};

} // namespace UI
