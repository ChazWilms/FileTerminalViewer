#pragma once
#include <string>
#include <unordered_map>

// Config manages all user settings loaded from TFV_config_template.txt.
// Keybindings are stored as action -> key mappings where the key is
// either a printable ASCII character or a special key code string
// (e.g. "UP", "DOWN", "F1", "ENTER").

namespace Config {

// Special key name constants used in the config file.
// Printable characters are written as-is (e.g.  move_down = h).
// Non-printable keys use these names          (e.g.  move_down = DOWN).
static constexpr const char* KEY_UP       = "UP";
static constexpr const char* KEY_DOWN     = "DOWN";
static constexpr const char* KEY_LEFT     = "LEFT";
static constexpr const char* KEY_RIGHT    = "RIGHT";
static constexpr const char* KEY_ENTER    = "ENTER";
static constexpr const char* KEY_ESCAPE   = "ESCAPE";
static constexpr const char* KEY_BACKSPACE= "BACKSPACE";
static constexpr const char* KEY_DELETE   = "DELETE";
static constexpr const char* KEY_HOME     = "HOME";
static constexpr const char* KEY_END      = "END";
static constexpr const char* KEY_PAGEUP   = "PAGEUP";
static constexpr const char* KEY_PAGEDOWN = "PAGEDOWN";

// Maps a special key name (as above) to its Platform::readKey() return value.
int specialKeyCode(const std::string& name);

// Maps a Platform::readKey() code back to a name string for display.
std::string keyName(int code);

struct Keybindings {
    int move_down    = 1080; // DOWN arrow
    int move_up      = 1072; // UP arrow
    int enter_dir    = 13;   // ENTER
    int parent_dir   = 1075; // LEFT arrow
    int command_line = ':';
    int quit         = 'q';
    int refresh      = 'r';
    int search       = '/';  // Enter live search mode
    int page_down    = 1081; // PAGE DOWN
    int page_up      = 1073; // PAGE UP
    int enter_file      = 1077; // RIGHT arrow — open file / enter dir (alias)
    int toggle_hidden   = 'H';  // toggle visibility of hidden files
    int jump_top        = 1071; // HOME — jump to first entry
    int jump_bottom     = 1079; // END  — jump to last entry
    int copy_entry      = 'c';  // copy selected entry to clipboard
    int paste_entry     = 'p';  // paste clipboard into current directory
};

struct Settings {
    Keybindings keys;
    bool        show_hidden = false;
};

// Load settings from the given file path.
// Returns true on success. On failure the default Settings are kept.
bool load(const std::string& path, Settings& out);

// Write a default config template to the given path.
bool writeTemplate(const std::string& path);

// Default config file name, looked up relative to the executable.
static constexpr const char* DEFAULT_CONFIG_NAME = "TFV_config_template.txt";

} // namespace Config
