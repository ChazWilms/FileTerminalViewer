#pragma once
#include <string>
#include <functional>
#include <vector>

// CommandLine handles the ":" prompt at the bottom of the screen.
// It receives a completed command string and dispatches to a handler.
// The handler callback is set by App so CommandLine stays decoupled
// from application state.

namespace UI {

struct CommandResult {
    bool        success;
    std::string message; // shown in status bar after command runs
};

// Callback type: receives the full command string, returns a result.
using CommandHandler = std::function<CommandResult(const std::string& cmd)>;

class CommandLine {
public:
    explicit CommandLine(CommandHandler handler);

    // Feed a keycode from Platform::readKey() to the command line.
    // Returns true while the command line is still active,
    // false when the user submits (Enter) or cancels (Escape).
    bool handleKey(int key);

    // Current text in the input buffer (excluding the leading ":").
    const std::string& buffer() const { return m_buffer; }

    // True if the command line is waiting for input.
    bool isActive() const { return m_active; }

    // Activate the command line (clear buffer, set active).
    void activate();

    // Deactivate without running a command.
    void cancel();

    // Last result message (set after a command runs).
    const std::string& lastMessage() const { return m_lastMessage; }

private:
    CommandHandler           m_handler;
    std::string              m_buffer;
    bool                     m_active      = false;
    std::string              m_lastMessage;

    // Command history (oldest first). Up arrow walks backwards.
    std::vector<std::string> m_history;
    int                      m_historyIdx  = -1; // -1 = not browsing
    std::string              m_savedBuffer;       // buffer before browsing started
};

} // namespace UI
