#include "CommandLine.h"

static constexpr int KEY_ENTER     = 13;
static constexpr int KEY_ESCAPE    = 27;
static constexpr int KEY_BACKSPACE = 8;

// Extended keys from Platform::readKey() (1000 + _getch() scan code).
static constexpr int KEY_UP    = 1072; // 0xE0 + 72
static constexpr int KEY_DOWN  = 1080; // 0xE0 + 80

namespace UI {

CommandLine::CommandLine(CommandHandler handler)
    : m_handler(std::move(handler)) {}

void CommandLine::activate() {
    m_buffer.clear();
    m_active      = true;
    m_historyIdx  = -1;
    m_savedBuffer.clear();
    m_lastMessage.clear();
}

void CommandLine::cancel() {
    m_buffer.clear();
    m_active     = false;
    m_historyIdx = -1;
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
            // Push to history (avoid consecutive duplicates).
            if (m_history.empty() || m_history.back() != m_buffer) {
                m_history.push_back(m_buffer);
            }
            CommandResult result = m_handler(m_buffer);
            m_lastMessage = result.message;
        }
        m_buffer.clear();
        m_historyIdx = -1;
        return false;
    }

    if (key == KEY_BACKSPACE) {
        if (!m_buffer.empty()) m_buffer.pop_back();
        m_historyIdx = -1; // any edit exits history browsing
        return true;
    }

    if (key == KEY_UP) {
        if (m_history.empty()) return true;
        if (m_historyIdx == -1) {
            // Start browsing: save current buffer.
            m_savedBuffer = m_buffer;
            m_historyIdx  = static_cast<int>(m_history.size()) - 1;
        } else if (m_historyIdx > 0) {
            --m_historyIdx;
        }
        m_buffer = m_history[m_historyIdx];
        return true;
    }

    if (key == KEY_DOWN) {
        if (m_historyIdx == -1) return true;
        if (m_historyIdx < static_cast<int>(m_history.size()) - 1) {
            ++m_historyIdx;
            m_buffer = m_history[m_historyIdx];
        } else {
            // Past the end of history — restore the saved buffer.
            m_historyIdx = -1;
            m_buffer     = m_savedBuffer;
        }
        return true;
    }

    // Accept printable ASCII only.
    if (key >= 32 && key < 127) {
        m_buffer += static_cast<char>(key);
        m_historyIdx = -1; // typing exits history browsing
    }

    return true;
}

} // namespace UI
