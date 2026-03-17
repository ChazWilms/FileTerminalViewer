#include "CommandLine.h"

static constexpr int KEY_ENTER     = 13;
static constexpr int KEY_ESCAPE    = 27;
static constexpr int KEY_BACKSPACE = 8;

namespace UI {

CommandLine::CommandLine(CommandHandler handler)
    : m_handler(std::move(handler)) {}

void CommandLine::activate() {
    m_buffer.clear();
    m_active = true;
    m_lastMessage.clear();
}

void CommandLine::cancel() {
    m_buffer.clear();
    m_active = false;
}

bool CommandLine::handleKey(int key) {
    if (!m_active) return false;

    if (key == KEY_ESCAPE) {
        cancel();
        return false;
    }

    if (key == KEY_ENTER) {
        m_active = false;
        if (!m_buffer.empty() && m_handler) {
            CommandResult result = m_handler(m_buffer);
            m_lastMessage = result.message;
        }
        m_buffer.clear();
        return false;
    }

    if (key == KEY_BACKSPACE) {
        if (!m_buffer.empty()) m_buffer.pop_back();
        return true;
    }

    // Accept printable ASCII only.
    if (key >= 32 && key < 127) {
        m_buffer += static_cast<char>(key);
    }

    return true;
}

} // namespace UI
