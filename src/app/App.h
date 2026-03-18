#pragma once
#include "config/Config.h"
#include "fs/FileSystem.h"
#include "ui/Renderer.h"
#include "ui/CommandLine.h"
#include "ui/LiveSearch.h"
#include <string>
#include <functional>

enum class SortField  { Name, Size, Date };
enum class SortOrder  { Asc, Desc };

// App owns all application state and the main event loop.
// It wires together Config, FileSystem, Renderer, and CommandLine.

class App {
public:
    explicit App(const Config::Settings& settings);

    // Enter the main event loop. Blocks until the user quits.
    void run();

private:
    // --- State ---
    Config::Settings     m_settings;
    std::string          m_currentPath;
    std::vector<FS::Entry> m_entries;
    int                  m_cursorIndex  = 0;
    int                  m_scrollOffset = 0;
    mutable int          m_searchScrollOffset = 0; // scroll offset during live search
    std::string          m_statusMsg;
    std::string          m_clipboard;   // path copied to clipboard
    bool                 m_clipboardIsDir = false;
    bool                 m_running      = true;
    SortField            m_sortField    = SortField::Name;
    SortOrder            m_sortOrder    = SortOrder::Asc;

    UI::CommandLine      m_cmdLine;
    UI::LiveSearch       m_liveSearch;

    // Pending confirmation: if non-empty, the next keypress is y/n.
    std::string              m_confirmMsg;
    std::function<void()>    m_confirmAction;

    // --- Input handling ---
    void handleKey(int key);
    void requestConfirm(const std::string& msg, std::function<void()> action);

    // --- Navigation ---
    void navigateTo(const std::string& path);
    void refresh();
    void moveCursor(int delta);
    void reSort();
    void pageMove(int direction);
    void clampScroll();

    // --- File operations ---
    void copySelected();
    void pasteClipboard();
    void deleteSelected();

    // --- Command dispatch ---
    UI::CommandResult dispatchCommand(const std::string& cmd);

    // --- Rendering ---
    UI::RenderState buildRenderState() const;
    void redraw();
    void setStatus(const std::string& msg);
};
