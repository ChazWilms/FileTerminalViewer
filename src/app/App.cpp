#include "App.h"
#include "platform/Console.h"
#include "ui/Renderer.h"
#include <sstream>
#include <algorithm>
#include <functional>
#include <vector>
#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <shellapi.h>
#endif

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
    // Priority 0: confirmation prompt — y confirms, anything else cancels.
    if (!m_confirmMsg.empty()) {
        if (key == 'y' || key == 'Y') {
            auto action = std::move(m_confirmAction);
            m_confirmMsg.clear();
            m_confirmAction = nullptr;
            action();
        } else {
            m_confirmMsg.clear();
            m_confirmAction = nullptr;
            setStatus("Cancelled.");
        }
        return;
    }

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
                // Replace the visible list with the filtered result.
                // Land the cursor on whatever the user had highlighted.
                m_entries      = m_liveSearch.matches();
                m_cursorIndex  = m_liveSearch.selectedIndex();
                m_scrollOffset = 0;
                clampScroll();
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
    else if (key == k.enter_dir || key == k.enter_file) {
        if (!m_entries.empty()) {
            const auto& e = m_entries[m_cursorIndex];
            if (e.type == FS::EntryType::Directory) {
                navigateTo(FS::joinPath(m_currentPath, e.name));
            } else {
                // Open file with default application.
#ifdef PLATFORM_WINDOWS
                std::string full = FS::joinPath(m_currentPath, e.name);
                ShellExecuteA(nullptr, "open", full.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                setStatus("Opened: " + e.name);
#endif
            }
        }
    }
    else if (key == k.parent_dir)   { navigateTo(FS::parentPath(m_currentPath)); }
    else if (key == k.command_line) { m_cmdLine.activate(); setStatus(""); }
    else if (key == k.refresh)      { refresh(); setStatus("Refreshed."); }
    else if (key == k.search)       {
        // Always search against the full directory listing.
        m_entries = FS::listDirectory(m_currentPath);
        m_searchScrollOffset = 0;
        m_liveSearch.activate(m_entries);
        setStatus("");
    }
    else if (key == k.page_down)    { pageMove(+1); }
    else if (key == k.page_up)      { pageMove(-1); }
    else if (key == k.jump_top)     { moveCursor(-static_cast<int>(m_entries.size())); }
    else if (key == k.jump_bottom)  { moveCursor(+static_cast<int>(m_entries.size())); }
    else if (key == k.toggle_hidden) {
        m_settings.show_hidden = !m_settings.show_hidden;
        refresh();
        setStatus(m_settings.show_hidden ? "Hidden files shown." : "Hidden files hidden.");
    }
    else if (key == k.copy_entry)   { copySelected(); }
    else if (key == k.paste_entry)  { pasteClipboard(); }
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
    auto all = FS::listDirectory(m_currentPath);
    if (!m_settings.show_hidden) {
        m_entries.clear();
        for (auto& e : all)
            if (!e.hidden)
                m_entries.push_back(std::move(e));
    } else {
        m_entries = std::move(all);
    }
    reSort();
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
    m_statusMsg.clear();
}

void App::pageMove(int direction) {
    if (m_entries.empty()) return;
    auto dim = Platform::getDimensions();
    int pageSize = std::max(1, dim.height - 3);
    moveCursor(direction * pageSize);
}

void App::reSort() {
    auto cmp = [this](const FS::Entry& a, const FS::Entry& b) -> bool {
        // Directories always sort before files regardless of field.
        bool aDir = (a.type == FS::EntryType::Directory);
        bool bDir = (b.type == FS::EntryType::Directory);
        if (aDir != bDir) return aDir > bDir;

        bool less;
        switch (m_sortField) {
        case SortField::Size:
            less = a.size < b.size;
            break;
        case SortField::Date:
            less = a.modified < b.modified;
            break;
        default: { // Name
            std::string la = a.name, lb = b.name;
            std::transform(la.begin(), la.end(), la.begin(), ::tolower);
            std::transform(lb.begin(), lb.end(), lb.begin(), ::tolower);
            less = la < lb;
            break;
        }
        }
        return (m_sortOrder == SortOrder::Asc) ? less : !less;
    };
    std::sort(m_entries.begin(), m_entries.end(), cmp);
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
        if (arg.empty()) return {false, "Usage: cd <path>"};
        std::string target = arg;
        // Expand ~ to %USERPROFILE%.
        if (target == "~" || (target.size() >= 2 && (target[1] == '/' || target[1] == '\\'))) {
#ifdef PLATFORM_WINDOWS
            const char* home = std::getenv("USERPROFILE");
            if (home) target = std::string(home) + (target == "~" ? "" : target.substr(1));
#endif
        }
        // Treat as absolute only if it looks like a drive path (e.g. C:\).
        bool isAbsolute = (target.size() >= 2 && target[1] == ':');
        if (!isAbsolute) target = FS::joinPath(m_currentPath, target);
        navigateTo(target);
        return {true, "Changed directory."};
    }

    // ---- mkdir ----
    if (cmd == "mkdir") {
        if (arg.empty()) return {false, "Usage: mkdir <name>"};
        std::string target = FS::joinPath(m_currentPath, arg);
        if (FS::exists(target)) return {false, "Already exists: " + arg};
        bool ok = FS::makeDirectory(target);
        if (ok) refresh();
        return {ok, ok ? "Directory created." : "Failed to create directory."};
    }

    // ---- mkf ----
    if (cmd == "mkf") {
        if (arg.empty()) return {false, "Usage: mkf <name>"};
        std::string target = FS::joinPath(m_currentPath, arg);
        if (FS::exists(target)) return {false, "Already exists: " + arg};
        bool ok = FS::makeFile(target);
        if (ok) refresh();
        return {ok, ok ? "File created." : "Failed to create file."};
    }

    // ---- del / rm — unified delete (files and directories) ----
    if (cmd == "del" || cmd == "rm") {
        std::string name, target;
        bool isDir;
        if (arg.empty()) {
            if (m_entries.empty()) return {false, "Nothing to delete."};
            const auto& e = m_entries[m_cursorIndex];
            name   = e.name;
            target = FS::joinPath(m_currentPath, name);
            isDir  = (e.type == FS::EntryType::Directory);
        } else {
            name   = arg;
            target = FS::joinPath(m_currentPath, arg);
            isDir  = FS::isDirectory(target);
        }
        std::string prompt = (isDir ? "Delete dir \"" : "Delete \"") + name + "\"? [y/n]";
        requestConfirm(prompt, [this, target, name, isDir]() {
            bool ok = isDir ? FS::deleteDirectoryRecursive(target) : FS::deleteFile(target);
            refresh();
            setStatus(ok ? "Deleted: " + name : "Delete failed: " + name);
        });
        return {true, ""};
    }

    // ---- nvim ----
    if (cmd == "nvim") {
        std::string target = arg.empty()
            ? (m_entries.empty() ? "" : FS::joinPath(m_currentPath, m_entries[m_cursorIndex].name))
            : FS::joinPath(m_currentPath, arg);
        if (target.empty()) return {false, "No file selected."};
        Platform::shutdown();
#ifdef PLATFORM_WINDOWS
        // Use CreateProcess to avoid shell injection via filenames with quotes.
        {
            // Build the command line with proper quoting.
            std::string cmdLine = "nvim \"" + target + "\"";
            // Replace any embedded double-quotes in the path with escaped versions.
            std::string safePath = target;
            size_t pos = 0;
            while ((pos = safePath.find('"', pos)) != std::string::npos) {
                safePath.replace(pos, 1, "\\\"");
                pos += 2;
            }
            std::string safeCmd = "nvim \"" + safePath + "\"";
            std::vector<char> buf(safeCmd.begin(), safeCmd.end());
            buf.push_back('\0');
            STARTUPINFOA si = {};
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi = {};
            if (CreateProcessA(nullptr, buf.data(), nullptr, nullptr,
                               FALSE, 0, nullptr, nullptr, &si, &pi)) {
                WaitForSingleObject(pi.hProcess, INFINITE);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        }
#else
        std::system(("nvim \"" + target + "\"").c_str());
#endif
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

    // ---- rename ----
    if (cmd == "rename") {
        if (arg.empty()) return {false, "Usage: rename <new_name>"};
        if (m_entries.empty()) return {false, "No entry selected."};
        const auto& e = m_entries[m_cursorIndex];
        std::string src = FS::joinPath(m_currentPath, e.name);
        std::string dst = FS::joinPath(m_currentPath, arg);
        if (FS::exists(dst)) return {false, "Rename failed: destination already exists."};
        bool ok = FS::moveEntry(src, dst);
        if (ok) refresh();
        return {ok, ok ? "Renamed to: " + arg : "Rename failed."};
    }

    // ---- mv ----
    if (cmd == "mv") {
        if (arg.empty()) return {false, "Usage: mv <destination_path>"};
        if (m_entries.empty()) return {false, "No entry selected."};
        const auto& e = m_entries[m_cursorIndex];
        std::string src = FS::joinPath(m_currentPath, e.name);
        // Resolve relative destination against current path.
        std::string dst = (arg.size() >= 2 && arg[1] == ':')
            ? arg
            : FS::joinPath(m_currentPath, arg);
        bool ok = FS::moveEntry(src, dst);
        if (ok) refresh();
        return {ok, ok ? "Moved to: " + arg : "Move failed."};
    }

    // ---- open ----
    if (cmd == "open") {
        std::string target = arg.empty()
            ? (m_entries.empty() ? "" : FS::joinPath(m_currentPath, m_entries[m_cursorIndex].name))
            : FS::joinPath(m_currentPath, arg);
        if (target.empty()) return {false, "No file selected."};
#ifdef PLATFORM_WINDOWS
        HINSTANCE result = ShellExecuteA(nullptr, "open",
            target.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        bool ok = (reinterpret_cast<intptr_t>(result) > 32);
        return {ok, ok ? "Opened: " + (arg.empty() ? m_entries[m_cursorIndex].name : arg)
                       : "Open failed."};
#else
        return {false, "open command not supported on this platform."};
#endif
    }

    // ---- sort ----
    if (cmd == "sort") {
        // Toggle direction if same field, otherwise switch field.
        if (arg == "name" || arg.empty()) {
            if (m_sortField == SortField::Name)
                m_sortOrder = (m_sortOrder == SortOrder::Asc) ? SortOrder::Desc : SortOrder::Asc;
            else { m_sortField = SortField::Name; m_sortOrder = SortOrder::Asc; }
        } else if (arg == "size") {
            if (m_sortField == SortField::Size)
                m_sortOrder = (m_sortOrder == SortOrder::Asc) ? SortOrder::Desc : SortOrder::Asc;
            else { m_sortField = SortField::Size; m_sortOrder = SortOrder::Asc; }
        } else if (arg == "date") {
            if (m_sortField == SortField::Date)
                m_sortOrder = (m_sortOrder == SortOrder::Asc) ? SortOrder::Desc : SortOrder::Asc;
            else { m_sortField = SortField::Date; m_sortOrder = SortOrder::Asc; }
        } else {
            return {false, "Usage: sort [name|size|date]"};
        }
        reSort();
        std::string fieldName = (m_sortField == SortField::Size) ? "size" :
                                (m_sortField == SortField::Date) ? "date" : "name";
        std::string orderName = (m_sortOrder == SortOrder::Asc) ? "asc" : "desc";
        return {true, "Sorted by " + fieldName + " (" + orderName + ")"};
    }

    // ---- help ----
    if (cmd == "help") {
        if (arg.empty()) {
            return {true,
                "cd  mkdir  mkf  del  rename  mv  nvim  open  sort  "
                "c=copy  p=paste  /=search  H=toggle hidden  initConfig  refreshConfig  q  "
                "| Type 'help <cmd>' for details"};
        }
        if (arg == "cd")            return {true, "cd <path> — navigate to a directory (relative or absolute)"};
        if (arg == "mkdir")         return {true, "mkdir <name> — create a new directory in the current folder"};
        if (arg == "mkf")           return {true, "mkf <name> — create a new empty file in the current folder"};
        if (arg == "del" || arg == "rm") return {true, "del/rm [name] — delete file or directory (recursive); omit name to delete the selected entry"};
        if (arg == "nvim")          return {true, "nvim [name] — open a file in neovim; omit name to open the selected entry"};
        if (arg == "rename")        return {true, "rename <new_name> — rename the selected entry in the current directory"};
        if (arg == "mv")            return {true, "mv <destination> — move the selected entry to a path (relative or absolute)"};
        if (arg == "open")          return {true, "open [name] — open a file with its default application; omit name to open the selected entry"};
        if (arg == "sort")           return {true, "sort [name|size|date] — sort the listing; repeat to toggle asc/desc"};
        if (arg == "initConfig")    return {true, "initConfig — write a default config template to the current directory"};
        if (arg == "refreshConfig") return {true, "refreshConfig — reload the config file from disk"};
        if (arg == "q" || arg == "quit") return {true, "q / quit — exit TFV"};
        return {false, "No help entry for: " + arg};
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
    s.confirmActive  = !m_confirmMsg.empty();
    {
        std::string field = (m_sortField == SortField::Size) ? "size" :
                            (m_sortField == SortField::Date) ? "date" : "name";
        std::string arrow = (m_sortOrder == SortOrder::Asc)  ? " ^" : " v";
        s.sortIndicator = field + arrow;
    }
    s.clipboard     = m_clipboard.empty() ? "" :
        ([&]{
            size_t sep = m_clipboard.find_last_of("\\/");
            return sep != std::string::npos
                ? m_clipboard.substr(sep + 1)
                : m_clipboard;
        }());

    // Show filtered matches while searching, full list otherwise.
    if (m_liveSearch.isActive()) {
        s.entries     = m_liveSearch.matches();
        s.cursorIndex = m_liveSearch.selectedIndex();
        // Clamp search scroll offset so the highlighted entry stays visible.
        auto dim = Platform::getDimensions();
        int listRows = std::max(1, dim.height - 3);
        if (s.cursorIndex < m_searchScrollOffset)
            m_searchScrollOffset = s.cursorIndex;
        else if (s.cursorIndex >= m_searchScrollOffset + listRows)
            m_searchScrollOffset = s.cursorIndex - listRows + 1;
        s.scrollOffset = m_searchScrollOffset;
        s.statusMsg    = "Search: " + m_liveSearch.query() +
                         "  (" + std::to_string(s.entries.size()) + " results)";
    } else {
        s.entries = m_entries;
        std::string countStr = std::to_string(s.entries.size()) + " item" +
                               (s.entries.size() == 1 ? "" : "s");
        if (!m_confirmMsg.empty()) {
            s.statusMsg = m_confirmMsg;
        } else {
            s.statusMsg = m_statusMsg.empty()
                ? countStr
                : m_statusMsg + "  [" + countStr + "]";
        }
    }

    return s;
}

void App::redraw() {
    UI::draw(buildRenderState());
}

void App::setStatus(const std::string& msg) {
    m_statusMsg = msg;
}

void App::requestConfirm(const std::string& msg, std::function<void()> action) {
    m_confirmMsg    = msg;
    m_confirmAction = std::move(action);
}
