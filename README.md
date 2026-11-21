# PROJECT-MAYHEM-CLI

> **Fight Club UNIX / C++20 90s CRT Terminal Simulator**  
> *Non-Canonical POSIX termios • RAII TerminalGuard • ANSI 24-bit Truecolor Phosphor • Live Linux /proc Kernel Telemetry*

[![License: MIT](https://img.shields.io/badge/License-MIT-00FF66.svg)](LICENSE)
[![C++20 Standard](https://img.shields.io/badge/C%2B%2B-20_Standard-00599C.svg)](https://en.cppreference.com/w/cpp/20)
[![POSIX termios](https://img.shields.io/badge/POSIX-termios_ioctl-FF1493.svg)](#)
[![Linux /proc Telemetry](https://img.shields.io/badge/Linux-/proc_kernel-FFBF00.svg)](#)

---

## Technical Architecture

**PROJECT-MAYHEM-CLI** showcases low-level UNIX and Operating Systems engineering:
- **Non-Canonical POSIX Control:** Disables `ECHO` and `ICANON` to process keystrokes unbuffered.
- **Dynamic Terminal Geometry:** Queries `ioctl(STDOUT_FILENO, TIOCGWINSZ, ...)` to adaptively scale borders and diagnostics.
- **Async-Signal-Safe Restoration:** Catches `SIGINT`/`SIGTERM` via RAII `TerminalGuard` to unconditionally restore user shell state (`\x1b[0m\x1b[?25h\x1b[?1049l`).
- **Phosphor CRT Simulation:** Emits 24-bit ANSI escape codes (`\x1b[38;2;0;255;102m` P31 green, `\x1b[38;2;255;20;147m` soap pink) with simulated CRT jitter and glitch drops.
- **Linux Kernel Telemetry:** Reads `/proc/uptime`, `/proc/loadavg`, and `/proc/meminfo`, transforming raw kernel statistics into industrial soap-factory telemetry.

---

## Quick Start

### Build & Run via Bash Wrapper:
```bash
chmod +x mayhem.sh
./mayhem.sh
```

### CLI Command Flags:
```bash
./mayhem.sh --rules   # Output classified directives
./mayhem.sh --marla   # Output encrypted phosphor intercept
./mayhem.sh --soap    # Live /proc memory and load telemetry
```

### Interactive Commands:
- `rules`: Display operational directives
- `marla`: Render encrypted terminal intercept
- `soap`: Inspect `/proc` kernel diagnostic telemetry
- `clear`: Purge CRT phosphor buffer
- `exit`: Cleanly restore shell and disconnect

---

## Author & License

- Engineered by **Prajwal Jogi** ([@prajwal918](https://github.com/prajwal918))
- Licensed under the [MIT License](LICENSE).
