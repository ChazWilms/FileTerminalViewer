#include "LiveSearch.h"
#include <algorithm>
#include <cctype>

static constexpr int KEY_ENTER     = 13;
static constexpr int KEY_ESCAPE    = 27;
static constexpr int KEY_BACKSPACE = 8;

namespace UI {

void LiveSearch::activate(const std::vector<FS::Entry>& allEntries) {
    m_allEntries = allEntries;
    m_query.clear();
    m_confirmed = false;
    m_active    = true;
    refilter();
}

void LiveSearch::cancel() {
    m_active    = false;
    m_confirmed = false;
    m_query.clear();
    m_matches.clear();
}

bool LiveSearch::handleKey(int key) {
    if (!m_active) return false;

    if (key == KEY_ESCAPE) {
        cancel();
        return false;
    }

    if (key == KEY_ENTER) {
        m_active    = false;
        m_confirmed = true;
        // Keep m_matches so the caller can use the filtered list.
        return false;
    }

    if (key == KEY_BACKSPACE) {
        if (!m_query.empty()) {
            m_query.pop_back();
            refilter();
        }
        return true;
    }

    // Accept printable ASCII only.
    if (key >= 32 && key < 127) {
        m_query += static_cast<char>(key);
        refilter();
    }

    return true;
}

void LiveSearch::refilter() {
    m_matches.clear();
    if (m_query.empty()) {
        m_matches = m_allEntries;
        return;
    }

    // Case-insensitive substring match.
    std::string lowerQuery = m_query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

    for (const auto& e : m_allEntries) {
        std::string lowerName = e.name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        if (lowerName.find(lowerQuery) != std::string::npos) {
            m_matches.push_back(e);
        }
    }
}

} // namespace UI
