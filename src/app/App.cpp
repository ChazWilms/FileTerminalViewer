#include "App.h"
#include "platform/Console.h"
#include "ui/Renderer.h"
#include <sstream>
#include <algorithm>

// -----------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------

App::App(const Config::Settings& settings)
    : m_settings(settings)
    , m_cmdLine([this](const std::string& cmd) { return dispatchCommand(cmd); })
{
    m_currentPath = FS::currentDirectory();
    refresh();
}

// -----------------------------------------------------------------------
// Main loop
// -----------------------------------------------------------------------

void App::run() {
    redraw();

    while (m_running) {
        int key = Platform::readKey();
        handleKey(key);
        redraw();
    }
}

// -----------------------------------------------------------------------
// Key routing
// -----------------------------------------------------------------------

void App::handleKey(int key) {
    // Priority 1: command line eats all keys while active.
    if (m_cmdLine.isActive()) {
        bool still_active = m_cmdLine.handleKey(key);
        if (!still_active && !m_cmdLine.lastMessage().empty()) {
            setStatus(m_cmdLine.lastMessage());
        }
        return;
    }

    // Priority 2: live search eats all keys while active.
    if (m_liveSearch.isActive()) {
        bool still_active = m_liveSearch.handleKey(key);
        if (!still_active) {
            if (m_liveSearch.confirmed() && !m_liveSearch.matches().empty()) {
                // Replace the visible list with the filtered result and reset cursor.
                m_entries      = m_liveSearch.matches();
                m_cursorIndex  = 0;
                m_scrollOffset = 0;
                setStatus("Search: " + m_liveSearch.query() +
                          "  (" + std::to_string(m_entries.size()) + " results)");
            } else {
                // Cancelled — restore full listing.
                refresh();
                setStatus("");
            }
        }
        return;
    }

    const auto& k = m_settings.keys;

    if      (key == k.quit)         { m_running = false; }
    else if (key == k.move_down)    { moveCursor(+1); }
    else if (key == k.move_up)      { moveCursor(-1); }
    else if (key == k.enter_dir)    {
        if (!m_entries.empty()) {
            const auto& e = m_entries[m_cursorIndex];
            if (e.type == FS::EntryType::Directory) {
                navigateTo(FS::joinPath(m_currentPath, e.name));
            }
        }
    }
    else if (key == k.parent_dir)   { navigateTo(FS::parentPath(m_currentPath)); }
    else if (key == k.command_line) { m_cmdLine.activate(); setStatus(""); }
    else if (key == k.refresh)      { refresh(); setStatus("Refreshed."); }
    else if (key == k.search)       {
        // Always search against the full directory listing.
        m_entries = FS::listDirectory(m_currentPath);
        m_liveSearch.activate(m_entries);
        setStatus("");
    }
    // Copy / paste — bound to 'c' and 'p' by default (not in config yet, coming soon).
    else if (key == 'c')            { copySelected(); }
    else if (key == 'p')            { pasteClipboard(); }
}

// -----------------------------------------------------------------------
// Navigation
// -----------------------------------------------------------------------

void App::navigateTo(const std::string& path) {
    std::string resolved = FS::resolvePath(path);
    if (resolved.empty() || !FS::isDirectory(resolved)) {
        setStatus("Cannot navigate to: " + path);
        return;
    }
    m_currentPath = resolved;
    m_cursorIndex  = 0;
    m_scrollOffset = 0;
    refresh();
    setStatus("");
}

void App::refresh() {
    m_entries = FS::listDirectory(m_currentPath);
    // Clamp cursor in case the directory shrank.
    if (!m_entries.empty()) {
        m_cursorIndex = std::min(m_cursorIndex, static_cast<int>(m_entries.size()) - 1);
    } else {
        m_cursorIndex = 0;
    }
    clampScroll();
}

void App::moveCursor(int delta) {
    if (m_entries.empty()) return;
    m_cursorIndex += delta;
    m_cursorIndex = std::max(0, std::min(m_cursorIndex, static_cast<int>(m_entries.size()) - 1));
    clampScroll();
}

void App::clampScroll() {
    auto dim = Platform::getDimensions();
    int listRows = dim.height - 3; // header (2) + footer (1)
    if (listRows < 1) listRows = 1;

    if (m_cursorIndex < m_scrollOffset) {
        m_scrollOffset = m_cursorIndex;
    } else if (m_cursorIndex >= m_scrollOffset + listRows) {
        m_scrollOffset = m_cursorIndex - listRows + 1;
    }
}

// -----------------------------------------------------------------------
// File operations
// -----------------------------------------------------------------------

void App::copySelected() {
    if (m_entries.empty()) return;
    const auto& e = m_entries[m_cursorIndex];
    m_clipboard      = FS::joinPath(m_currentPath, e.name);
    m_clipboardIsDir = (e.type == FS::EntryType::Directory);
    setStatus("Copied: " + e.name);
}

void App::pasteClipboard() {
    if (m_clipboard.empty()) {
        setStatus("Clipboard is empty.");
        return;
    }

    // Extract the base name from the clipboard path.
    std::string baseName = m_clipboard;
    size_t sep = baseName.find_last_of("\\/");
    if (sep != std::string::npos) baseName = baseName.substr(sep + 1);

    std::string dst = FS::joinPath(m_currentPath, baseName);

    if (FS::exists(dst)) {
        setStatus("Paste failed: destination already exists.");
        return;
    }

    bool ok = FS::copyEntry(m_clipboard, dst);
    if (ok) {
        refresh();
        setStatus("Pasted: " + baseName);
    } else {
        setStatus("Paste failed.");
    }
}

void App::deleteSelected() {
    if (m_entries.empty()) return;
    const auto& e = m_entries[m_cursorIndex];
    std::string path = FS::joinPath(m_currentPath, e.name);
    bool ok = false;

    if (e.type == FS::EntryType::Directory) {
        ok = FS::deleteDirectoryRecursive(path);
    } else {
        ok = FS::deleteFile(path);
    }

    if (ok) {
        refresh();
        setStatus("Deleted: " + e.name);
    } else {
        setStatus("Delete failed: " + e.name);
    }
}

// -----------------------------------------------------------------------
// Command dispatch
// -----------------------------------------------------------------------

UI::CommandResult App::dispatchCommand(const std::string& rawCmd) {
    // Split on spaces: first token is the command, rest are args.
    std::istringstream ss(rawCmd);
    std::string cmd;
    ss >> cmd;

    std::string arg;
    std::getline(ss >> std::ws, arg); // remainder after command

    // ---- cd ----
    if (cmd == "cd") {
        std::string target = arg.empty() ? m_currentPath : arg;
        // Support relative paths.
        if (!arg.empty() && arg[1] != ':') {
            target = FS::joinPath(m_currentPath, arg);
        }
        navigateTo(target);
        return {true, "Changed directory."};
    }

    // ---- mkdir ----
    if (cmd == "mkdir") {
        if (arg.empty()) return {false, "Usage: mkdir <name>"};
        FS::makeDirectory(FS::joinPath(m_currentPath, arg));
        refresh();
        return {true, "Directory created."};
    }

    // ---- mkf ----
    if (cmd == "mkf") {
        if (arg.empty()) return {false, "Usage: mkf <name>"};
        FS::makeFile(FS::joinPath(m_currentPath, arg));
        refresh();
        return {true, "File created."};
    }

    // ---- del ----
    if (cmd == "del") {
        if (arg.empty()) {
            deleteSelected();
            return {true, m_statusMsg};
        }
        bool ok = FS::deleteFile(FS::joinPath(m_currentPath, arg));
        refresh();
        return {ok, ok ? "Deleted." : "Delete failed."};
    }

    // ---- rm ----
    if (cmd == "rm") {
        if (arg.empty()) return {false, "Usage: rm <dir>"};
        bool ok = FS::deleteDirectory(FS::joinPath(m_currentPath, arg));
        refresh();
        return {ok, ok ? "Removed." : "Remove failed (directory may not be empty)."};
    }

    // ---- nvim ----
    if (cmd == "nvim") {
        std::string target = arg.empty()
            ? (m_entries.empty() ? "" : FS::joinPath(m_currentPath, m_entries[m_cursorIndex].name))
            : FS::joinPath(m_currentPath, arg);
        if (target.empty()) return {false, "No file selected."};
        std::string sysCmd = "nvim \"" + target + "\"";
        Platform::shutdown();
        std::system(sysCmd.c_str());
        Platform::init();
        refresh();
        return {true, "Returned from nvim."};
    }

    // ---- initConfig / refreshConfig ----
    if (cmd == "initConfig") {
        bool ok = Config::writeTemplate(Config::DEFAULT_CONFIG_NAME);
        return {ok, ok ? "Config template written." : "Failed to write config."};
    }
    if (cmd == "refreshConfig") {
        Config::load(Config::DEFAULT_CONFIG_NAME, m_settings);
        return {true, "Config reloaded."};
    }

    // ---- help ----
    if (cmd == "help") {
        return {true,
            "Commands: cd, mkdir, mkf, del, rm, nvim, initConfig, refreshConfig, help, q"};
    }

    // ---- q (quit from command line) ----
    if (cmd == "q" || cmd == "quit") {
        m_running = false;
        return {true, ""};
    }

    return {false, "Unknown command: " + cmd};
}

// -----------------------------------------------------------------------
// Rendering helpers
// -----------------------------------------------------------------------

UI::RenderState App::buildRenderState() const {
    UI::RenderState s;
    s.currentPath   = m_currentPath;
    s.cursorIndex   = m_cursorIndex;
    s.scrollOffset  = m_scrollOffset;
    s.cmdLineActive = m_cmdLine.isActive();
    s.cmdLineBuffer = m_cmdLine.buffer();
    s.searchActive  = m_liveSearch.isActive();
    s.searchQuery   = m_liveSearch.query();
    s.clipboard     = m_clipboard.empty() ? "" :
        ([&]{
            size_t sep = m_clipboard.find_last_of("\\/");
            return sep != std::string::npos
                ? m_clipboard.substr(sep + 1)
                : m_clipboard;
        }());

    // Show filtered matches while searching, full list otherwise.
    if (m_liveSearch.isActive()) {
        s.entries  = m_liveSearch.matches();
        s.statusMsg = "Search: " + m_liveSearch.query() +
                      "  (" + std::to_string(s.entries.size()) + " results)";
    } else {
        s.entries  = m_entries;
        s.statusMsg = m_statusMsg;
    }

    return s;
}

void App::redraw() {
    UI::draw(buildRenderState());
}

void App::setStatus(const std::string& msg) {
    m_statusMsg = msg;
}
