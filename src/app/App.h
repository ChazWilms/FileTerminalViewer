#pragma once
#include "config/Config.h"
#include "fs/FileSystem.h"
#include "ui/Renderer.h"
#include "ui/CommandLine.h"
#include "ui/LiveSearch.h"
#include <string>

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
    std::string          m_statusMsg;
    std::string          m_clipboard;   // path copied to clipboard
    bool                 m_clipboardIsDir = false;
    bool                 m_running      = true;

    UI::CommandLine      m_cmdLine;
    UI::LiveSearch       m_liveSearch;

    // --- Input handling ---
    void handleKey(int key);
    void handleNavKey(int key);

    // --- Navigation ---
    void navigateTo(const std::string& path);
    void refresh();
    void moveCursor(int delta);
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
