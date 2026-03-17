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
    int move_down    = 'h';
    int move_up      = 't';
    int enter_dir    = 's';
    int parent_dir   = 'a';
    int command_line = ':';
    int quit         = 'q';
    int select       = ' ';   // Space bar selects/deselects an entry
    int refresh      = 'r';
    int search       = '/';   // Enter live search mode
};

struct Settings {
    Keybindings keys;
    // Future general settings (e.g. show_hidden, color_scheme) go here.
};

// Load settings from the given file path.
// Returns true on success. On failure the default Settings are kept.
bool load(const std::string& path, Settings& out);

// Write a default config template to the given path.
bool writeTemplate(const std::string& path);

// Default config file name, looked up relative to the executable.
static constexpr const char* DEFAULT_CONFIG_NAME = "TFV_config_template.txt";

} // namespace Config
