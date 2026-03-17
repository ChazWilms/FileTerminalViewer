#include "platform/Console.h"
#include "config/Config.h"
#include "app/App.h"
#include <iostream>
#include <fstream>
#include <stdexcept>

// Locate the config file next to the executable (or fall back to CWD).
static std::string findConfigPath() {
    // Try working directory first.
    std::string local = Config::DEFAULT_CONFIG_NAME;
    std::ifstream test(local);
    if (test.good()) return local;
    return local; // return the name anyway; Config::load will just fail silently
}

int main() {
    try {
        // 1. Load configuration (falls back to defaults if file not found).
        Config::Settings settings;
        std::string cfgPath = findConfigPath();
        Config::load(cfgPath, settings);

        // 2. Initialize the console (raw mode, hide cursor, etc.).
        Platform::init();

        // 3. Run the application.
        App app(settings);
        app.run();

        // 4. Restore the console.
        Platform::shutdown();

    } catch (const std::exception& ex) {
        // Ensure console is restored even on error.
        try { Platform::shutdown(); } catch (...) {}
        std::cerr << "TFV fatal error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
