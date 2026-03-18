#include "Config.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

// Extended key codes returned by Platform::readKey() for non-ASCII keys.
// These must stay in sync with ConsoleWindows.cpp (1000 + scan code).
// Scan codes from _getch() extended sequences on Windows:
static constexpr int EXT_BASE   = 1000;
static constexpr int SCAN_UP    = 72;
static constexpr int SCAN_DOWN  = 80;
static constexpr int SCAN_LEFT  = 75;
static constexpr int SCAN_RIGHT = 77;
static constexpr int SCAN_HOME  = 71;
static constexpr int SCAN_END   = 79;
static constexpr int SCAN_PGUP  = 73;
static constexpr int SCAN_PGDN  = 81;
static constexpr int SCAN_DEL   = 83;

static constexpr int CODE_ENTER     = 13;
static constexpr int CODE_ESCAPE    = 27;
static constexpr int CODE_BACKSPACE = 8;

namespace Config {

// -----------------------------------------------------------------------
// Key name <-> code translation
// -----------------------------------------------------------------------

int specialKeyCode(const std::string& name) {
    if (name == KEY_UP)        return EXT_BASE + SCAN_UP;
    if (name == KEY_DOWN)      return EXT_BASE + SCAN_DOWN;
    if (name == KEY_LEFT)      return EXT_BASE + SCAN_LEFT;
    if (name == KEY_RIGHT)     return EXT_BASE + SCAN_RIGHT;
    if (name == KEY_HOME)      return EXT_BASE + SCAN_HOME;
    if (name == KEY_END)       return EXT_BASE + SCAN_END;
    if (name == KEY_PAGEUP)    return EXT_BASE + SCAN_PGUP;
    if (name == KEY_PAGEDOWN)  return EXT_BASE + SCAN_PGDN;
    if (name == KEY_DELETE)    return EXT_BASE + SCAN_DEL;
    if (name == KEY_ENTER)     return CODE_ENTER;
    if (name == KEY_ESCAPE)    return CODE_ESCAPE;
    if (name == KEY_BACKSPACE)  return CODE_BACKSPACE;
    return -1; // unknown
}

std::string keyName(int code) {
    if (code == EXT_BASE + SCAN_UP)    return KEY_UP;
    if (code == EXT_BASE + SCAN_DOWN)  return KEY_DOWN;
    if (code == EXT_BASE + SCAN_LEFT)  return KEY_LEFT;
    if (code == EXT_BASE + SCAN_RIGHT) return KEY_RIGHT;
    if (code == EXT_BASE + SCAN_HOME)  return KEY_HOME;
    if (code == EXT_BASE + SCAN_END)   return KEY_END;
    if (code == EXT_BASE + SCAN_PGUP)  return KEY_PAGEUP;
    if (code == EXT_BASE + SCAN_PGDN)  return KEY_PAGEDOWN;
    if (code == EXT_BASE + SCAN_DEL)   return KEY_DELETE;
    if (code == CODE_ENTER)     return KEY_ENTER;
    if (code == CODE_ESCAPE)    return KEY_ESCAPE;
    if (code == CODE_BACKSPACE) return KEY_BACKSPACE;
    if (code >= 32 && code < 127) {
        return std::string(1, static_cast<char>(code));
    }
    return "?";
}

// -----------------------------------------------------------------------
// Parsing helpers
// -----------------------------------------------------------------------

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

// Parse "key = value" or "key=value". Returns false if malformed.
static bool parseLine(const std::string& line, std::string& key, std::string& value) {
    size_t eq = line.find('=');
    if (eq == std::string::npos) return false;
    key   = trim(line.substr(0, eq));
    value = trim(line.substr(eq + 1));
    return !key.empty() && !value.empty();
}

// Convert a config value string to a key code int.
// Single printable char -> its ASCII value.
// Special name        -> specialKeyCode() result.
static int parseKeyValue(const std::string& val) {
    if (val.size() == 1 && val[0] >= 32 && val[0] < 127) {
        return static_cast<int>(val[0]);
    }
    int code = specialKeyCode(toUpper(val));
    return code; // -1 if unrecognised — caller keeps default
}

// -----------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------

bool load(const std::string& path, Settings& out) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        // Strip comments (# or ;)
        size_t comment = line.find_first_of("#;");
        if (comment != std::string::npos) line = line.substr(0, comment);

        std::string key, value;
        if (!parseLine(line, key, value)) continue;

        // General (non-key) settings handled before key parsing.
        if (key == "show_hidden") {
            out.show_hidden = (value == "true" || value == "1");
            continue;
        }

        int code = parseKeyValue(value);
        if (code < 0) continue; // unrecognised value — skip

        // Map config keys to Keybindings fields.
        if      (key == "move_down")     out.keys.move_down     = code;
        else if (key == "move_up")       out.keys.move_up       = code;
        else if (key == "enter_dir")     out.keys.enter_dir     = code;
        else if (key == "parent_dir")    out.keys.parent_dir    = code;
        else if (key == "command_line")  out.keys.command_line  = code;
        else if (key == "quit")          out.keys.quit          = code;
        else if (key == "refresh")       out.keys.refresh       = code;
        else if (key == "search")        out.keys.search        = code;
        else if (key == "page_down")     out.keys.page_down     = code;
        else if (key == "page_up")       out.keys.page_up       = code;
        else if (key == "enter_file")    out.keys.enter_file    = code;
        else if (key == "toggle_hidden") out.keys.toggle_hidden = code;
        else if (key == "jump_top")      out.keys.jump_top      = code;
        else if (key == "jump_bottom")   out.keys.jump_bottom   = code;
        else if (key == "copy_entry")    out.keys.copy_entry    = code;
        else if (key == "paste_entry")   out.keys.paste_entry   = code;
    }
    return true;
}

bool writeTemplate(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file <<
        "# TFV Configuration File\n"
        "# ----------------------\n"
        "# Set keybindings by assigning a single character or a special key name.\n"
        "#\n"
        "# Special key names: UP, DOWN, LEFT, RIGHT, ENTER, ESCAPE,\n"
        "#                    BACKSPACE, DELETE, HOME, END, PAGEUP, PAGEDOWN\n"
        "#\n"
        "# Example:  move_down = j\n"
        "# Example:  enter_dir = ENTER\n"
        "\n"
        "[keybindings]\n"
        "move_down    = DOWN\n"
        "move_up      = UP\n"
        "enter_dir    = ENTER\n"
        "parent_dir   = LEFT\n"
        "command_line = :\n"
        "quit         = q\n"
        "refresh      = r\n"
        "search       = /\n"
        "page_down    = PAGEDOWN\n"
        "page_up      = PAGEUP\n"
        "enter_file      = RIGHT\n"
        "toggle_hidden   = H\n"
        "jump_top        = HOME\n"
        "jump_bottom     = END\n"
        "copy_entry      = c\n"
        "paste_entry     = p\n"
"\n"
        "[general]\n"
        "show_hidden     = false\n"
        "\n";

    return true;
}

} // namespace Config
