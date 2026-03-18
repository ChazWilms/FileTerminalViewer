# TFV — Terminal File Viewer

A keyboard-driven terminal file browser for Windows, written in C++17.

![Platform](https://img.shields.io/badge/platform-Windows-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B17-orange)
![Build](https://img.shields.io/badge/build-CMake%20%2B%20MinGW-green)

## Features

- Browse directories with arrow keys — no mouse needed
- Live search with real-time filtering and result navigation
- Sort by name, size, or date (ascending/descending toggle)
- Open files with the system default application
- Launch files in Neovim directly from the browser
- Copy/paste entries between directories
- Toggle hidden file visibility (respects `FILE_ATTRIBUTE_HIDDEN`)
- Command history in the `:` prompt (up/down arrow)
- Command-line style operations: `cd`, `mkdir`, `mkf`, `del`, `rm`, `nvim`
- Confirmation prompt for destructive operations
- Configurable keybindings via config file
- Restores your terminal on exit (no screen wipe)
- Dynamic column widths — adapts to terminal size and resizes live

## Default Keybindings

| Key | Action |
|-----|--------|
| `↑` / `↓` | Move cursor |
| `→` / `Enter` | Open file or enter directory |
| `←` | Go to parent directory |
| `Page Up` / `Page Down` | Scroll one page |
| `Home` / `End` | Jump to first/last entry |
| `/` | Enter live search mode |
| `:` | Open command line |
| `H` | Toggle hidden files |
| `c` | Copy selected entry |
| `p` | Paste into current directory |
| `r` | Refresh directory |
| `q` | Quit |

### In live search mode
| Key | Action |
|-----|--------|
| `↑` / `↓` | Move through search results |
| `Escape` | Exit search |

## Commands

| Command | Description |
|---------|-------------|
| `:cd <path>` | Change directory (supports `~` for home) |
| `:mkdir <name>` | Create a new directory |
| `:mkf <name>` | Create a new file |
| `:del <name>` | Delete a file or directory (with confirmation) |
| `:rm <name>` | Alias for `del` |
| `:nvim <name>` | Open a file in Neovim |
| `:sort [name\|size\|date]` | Sort entries (repeat to toggle asc/desc) |

## Building

**Requirements:** Windows, CMake 3.16+, MinGW-w64 (GCC)

```bat
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
```

The binary is output to `build/bin/tfv.exe`.

A `build.sh` script is also included for building from Git Bash or MSYS2:

```bash
./build.sh
```

## Configuration

On first run, TFV writes a default config to `TFV_config_template.txt` in the same directory as the executable. Edit it to customize keybindings and settings.

```ini
[general]
show_hidden = false

[keybindings]
move_up    = UP
move_down  = DOWN
enter_dir  = ENTER
parent_dir = LEFT
enter_file = RIGHT
page_up    = PAGEUP
page_down  = PAGEDOWN
jump_top   = HOME
jump_bottom = END
search     = /
command_line = :
quit       = q
refresh    = r
toggle_hidden = H
copy_entry = c
paste_entry = p
```

## Architecture

```
src/
├── app/        # App — main loop, input handling, state
├── config/     # Config — loads and writes config file
├── fs/         # FileSystem — directory listing, file ops
├── platform/   # Console abstraction (Windows)
└── ui/         # Renderer, LiveSearch, CommandLine
```

The app follows an MVC-style split: `App` owns state, `Renderer` is a pure view fed via a `RenderState` snapshot, and `Platform` abstracts all console I/O.

## License

MIT
