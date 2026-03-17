# TFV Configuration Guide

TFV loads its settings from `TFV_config_template.txt` in the same directory as the executable.
If the file doesn't exist, all defaults are used. Run `:initConfig` inside TFV to generate a
fresh template.

## File Format

- Plain text, INI-inspired `key = value` pairs.
- `#` and `;` begin line comments.
- Sections like `[keybindings]` are parsed but currently optional.

## Keybinding Values

| Type | How to write it | Example |
|------|----------------|---------|
| Printable character | Just the character | `move_down = j` |
| Special key | Use a key name (see below) | `enter_dir = ENTER` |

### Special Key Names

| Name | Key |
|------|-----|
| `UP` | Arrow Up |
| `DOWN` | Arrow Down |
| `LEFT` | Arrow Left |
| `RIGHT` | Arrow Right |
| `ENTER` | Enter / Return |
| `ESCAPE` | Escape |
| `BACKSPACE` | Backspace |
| `DELETE` | Delete |
| `HOME` | Home |
| `END` | End |
| `PAGEUP` | Page Up |
| `PAGEDOWN` | Page Down |

## Available Keybinding Options

| Option | Default | Description |
|--------|---------|-------------|
| `move_down` | `h` | Move cursor down |
| `move_up` | `t` | Move cursor up |
| `enter_dir` | `s` | Enter the selected directory |
| `parent_dir` | `a` | Go to parent directory |
| `command_line` | `:` | Open the command line |
| `quit` | `q` | Exit TFV |
| `select` | `Space` | Select/deselect an entry |
| `refresh` | `r` | Refresh the current directory |
| `search` | `/` | Activate live search mode |

## Live Search

Press `/` (or your configured `search` key) to enter live search mode:

- Type to filter the file list in real time (case-insensitive substring match)
- Matching text is highlighted **yellow** in each entry name
- The header bar turns **purple** while search is active
- `Enter` — confirm and keep the filtered list; navigate normally from there
- `Escape` — cancel and restore the full directory listing
- `Backspace` — delete the last character of your query

## Reloading Config at Runtime

Run `:refreshConfig` in the TFV command line to reload the config file without restarting.

## Generating a Fresh Config

Run `:initConfig` to write a default `TFV_config_template.txt` in the current directory.
