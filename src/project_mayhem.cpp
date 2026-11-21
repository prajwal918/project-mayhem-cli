// src/project_mayhem.cpp — Modern C++20 90s CRT POSIX Terminal Simulator
// Compile: g++ -std=c++20 -O2 -Wall -Wextra -o project_mayhem src/project_mayhem.cpp

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#define HAS_POSIX_TERMIOS 1
#else
#define HAS_POSIX_TERMIOS 0
#endif

namespace ansi {
constexpr std::string_view ALT_BUF_ON = "\x1b[?1049h";
constexpr std::string_view ALT_BUF_OFF = "\x1b[?1049l";
constexpr std::string_view CLEAR_HOME = "\x1b[2J\x1b[H";
constexpr std::string_view HIDE_CURSOR = "\x1b[?25l";
constexpr std::string_view SHOW_CURSOR = "\x1b[?25h";
constexpr std::string_view RESET = "\x1b[0m";
constexpr std::string_view BOLD = "\x1b[1m";
constexpr std::string_view PHOSPHOR = "\x1b[38;2;0;255;102m";      // #00FF66 Acid Green
constexpr std::string_view PHOSPHOR_LO = "\x1b[38;2;0;145;62m";    // Scanline dim green
constexpr std::string_view SOAP_PINK = "\x1b[38;2;255;20;147m";    // #FF1493 Soap Pink
constexpr std::string_view AMBER = "\x1b[38;2;255;191;0m";         // #FFBF00 Amber Alert
} // namespace ansi

#if HAS_POSIX_TERMIOS
static struct termios g_orig_termios {};
static std::atomic<bool> g_raw_active{false};
#endif
static std::atomic<bool> g_interrupted{false};

extern "C" void handle_signal(int) {
  g_interrupted.store(true);
#if HAS_POSIX_TERMIOS
  if (g_raw_active.load()) {
    tcsetattr(STDIN_FILENO, TCSANOW, &g_orig_termios);
  }
  const char restore_seq[] = "\x1b[0m\x1b[?25h\x1b[?1049l";
  [[maybe_unused]] auto _ = write(STDOUT_FILENO, restore_seq, sizeof(restore_seq) - 1);
#endif
  std::_Exit(130);
}

class TerminalGuard {
public:
  TerminalGuard() {
#if HAS_POSIX_TERMIOS
    is_tty_ = (isatty(STDIN_FILENO) == 1 && isatty(STDOUT_FILENO) == 1);
    if (!is_tty_) return;
    if (tcgetattr(STDIN_FILENO, &g_orig_termios) == 0) {
      struct termios raw = g_orig_termios;
      raw.c_lflag &= static_cast<tcflag_t>(~(ECHO | ICANON));
      raw.c_cc[VMIN] = 1;
      raw.c_cc[VTIME] = 0;
      if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0) {
        g_raw_active.store(true);
      }
    }
#else
    is_tty_ = false;
#endif
    std::cout << ansi::ALT_BUF_ON << ansi::CLEAR_HOME << std::flush;
  }

  ~TerminalGuard() {
#if HAS_POSIX_TERMIOS
    if (!is_tty_) return;
    if (g_raw_active.load()) {
      tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
      g_raw_active.store(false);
    }
#endif
    std::cout << ansi::RESET << ansi::SHOW_CURSOR << ansi::ALT_BUF_OFF << std::flush;
  }

  [[nodiscard]] bool is_tty() const noexcept { return is_tty_; }

  static std::pair<int, int> window_size() {
#if HAS_POSIX_TERMIOS
    struct winsize ws {};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
      return {static_cast<int>(ws.ws_col), static_cast<int>(ws.ws_row)};
    }
#endif
    return {80, 24};
  }

private:
  bool is_tty_{false};
};

void crt_stream(std::string_view text, std::string_view color = ansi::PHOSPHOR, int base_ms = 8, bool jitter = true) {
  static thread_local std::mt19937 rng{std::random_device{}()};
  std::uniform_int_distribution<int> glitch_roll(0, 48);
  constexpr std::string_view NOISE_GLYPHS = "#%*@!?/";

  std::cout << ansi::HIDE_CURSOR << color;
  for (char ch : text) {
    if (jitter && ch != '\n' && ch != ' ' && glitch_roll(rng) == 0) {
      char glitch = NOISE_GLYPHS[static_cast<std::size_t>(rng() % NOISE_GLYPHS.size())];
      std::cout << ansi::SOAP_PINK << glitch << "\b" << color << std::flush;
      std::this_thread::sleep_for(std::chrono::milliseconds(12));
    }
    std::cout << ch << std::flush;
    if (base_ms > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(ch == '\n' ? base_ms * 2 : base_ms));
    }
  }
  std::cout << ansi::RESET << ansi::SHOW_CURSOR << std::flush;
}

struct SoapTelemetry {
  double uptime_hours{0.0};
  double load_1m{0.0};
  long mem_total_mb{0};
  long mem_avail_mb{0};
  bool proc_available{false};
};

SoapTelemetry read_linux_telemetry() {
  SoapTelemetry t;
  std::ifstream up("/proc/uptime");
  if (up) {
    double secs = 0.0;
    up >> secs;
    t.uptime_hours = secs / 3600.0;
    t.proc_available = true;
  }
  std::ifstream load("/proc/loadavg");
  if (load) load >> t.load_1m;

  std::ifstream mem("/proc/meminfo");
  std::string key;
  long val_kb = 0;
  std::string unit;
  while (mem >> key >> val_kb >> unit) {
    if (key == "MemTotal:") t.mem_total_mb = val_kb / 1024;
    if (key == "MemAvailable:") t.mem_avail_mb = val_kb / 1024;
  }
  return t;
}

void print_banner() {
  auto [cols, rows] = TerminalGuard::window_size();
  std::ostringstream oss;
  oss << "====================================================================\n"
      << " PAPER STREET SOAP CO. // CRT TERMINAL v1.999 [TTY " << cols << "x" << rows << "]\n"
      << " NODE: PROJECT-MAYHEM | PHOSPHOR: P31 GREEN | MODE: RAW POSIX\n"
      << "====================================================================\n\n";
  crt_stream(oss.str(), ansi::PHOSPHOR, 2, false);
}

void cmd_rules() {
  crt_stream("\n[CLASSIFIED DIRECTIVE // RULES OF ENGAGEMENT]\n", ansi::AMBER, 6, true);
  const std::vector<std::string_view> rules = {
    " 01. You do not talk about Project Mayhem.\n",
    " 02. You DO NOT talk about Project Mayhem.\n",
    " 03. Always restore termios state before process exit.\n",
    " 04. Never commit API secrets to public repositories.\n",
    " 05. One process per responsibility; zero zombie children.\n",
    " 06. If this is your first night at the shell, you have to ship.\n\n"
  };
  for (auto line : rules) crt_stream(line, ansi::PHOSPHOR, 8, true);
}

void cmd_marla() {
  constexpr std::string_view portrait = R"(
       .---""""---.
     .-'  .-""""-.  '-.
    / .'  .----.  '. \    "SIGNAL INTERCEPT // ENCRYPTED NODE"
   / /   .' () '.   \ \   > Status: Telemetry established on /dev/tty
  | |   |  .--.  |   | |  > Threat: Entropy spike detected in buffer
  | |    \ '==' /    | |  > Advisory: Verify all upstream endpoints.
   \ \    '----'    / /
    '._  '.________.'  _.'
       '---......---'
)";
  crt_stream(portrait, ansi::SOAP_PINK, 2, true);
  std::cout << "\n";
}

void cmd_soap() {
  const auto t = read_linux_telemetry();
  const long used_mb = std::max(0L, t.mem_total_mb - t.mem_avail_mb);
  const double vat_pct = t.mem_total_mb > 0 ? (100.0 * static_cast<double>(used_mb) / t.mem_total_mb) : 42.0;

  constexpr std::string_view soap_bar = R"(
  _____________________________________________________
 /                                                     \
|    PAPER STREET SOAP COMPANY — BATCH #99-SAPONIFY    |
|              [ P R O J E C T   M A Y H E M ]         |
 \_____________________________________________________/
)";
  crt_stream(soap_bar, ansi::SOAP_PINK, 2, false);

  std::ostringstream report;
  report << std::fixed << std::setprecision(2);
  if (t.proc_available) {
    report << "\n [KERNEL /proc DIAGNOSTICS DISGUISED AS FACTORY TELEMETRY]\n"
           << "  > Vat Rendering Uptime  : " << t.uptime_hours << " hrs (/proc/uptime)\n"
           << "  > Lye Mixing Pressure   : " << t.load_1m << " load-avg (/proc/loadavg)\n"
           << "  > Glycerin Vat Fill     : " << used_mb << " MB / " << t.mem_total_mb << " MB (" << vat_pct << "% allocated)\n"
           << "  > Available Tallow      : " << t.mem_avail_mb << " MB (/proc/meminfo)\n\n";
  } else {
    report << "\n [TELEMETRY FALLBACK — NON-LINUX /proc UNAVAILABLE]\n"
           << "  > Lye Mixing Pressure   : NOMINAL (POSIX compatibility mode)\n"
           << "  > Glycerin Purity       : 99.4% (synthetic baseline)\n\n";
  }
  crt_stream(report.str(), ansi::PHOSPHOR, 4, true);
}

void cmd_help() {
  crt_stream(
    "\nAVAILABLE COMMANDS:\n"
    "  --rules   Display Paper Street classified directives\n"
    "  --marla   Render phosphor ASCII intercept\n"
    "  --soap    Dump live /proc memory & CPU telemetry as soap factory stats\n"
    "  --clear   Purge CRT phosphor buffer\n"
    "  --exit    Restore POSIX terminal and disconnect\n\n",
    ansi::PHOSPHOR_LO, 3, false);
}

bool dispatch(std::string_view cmd) {
  if (cmd == "--rules" || cmd == "rules") { cmd_rules(); return true; }
  if (cmd == "--marla" || cmd == "marla") { cmd_marla(); return true; }
  if (cmd == "--soap" || cmd == "soap") { cmd_soap(); return true; }
  if (cmd == "--help" || cmd == "help") { cmd_help(); return true; }
  if (cmd == "--clear" || cmd == "clear") { std::cout << ansi::CLEAR_HOME; print_banner(); return true; }
  if (cmd == "--exit" || cmd == "exit" || cmd == "quit") { return false; }
  crt_stream("  [ERR] Unknown directive. Type --help for authorized commands.\n", ansi::AMBER, 4, false);
  return true;
}

std::string read_raw_line() {
  std::string buf;
#if HAS_POSIX_TERMIOS
  char ch = 0;
  while (!g_interrupted.load() && read(STDIN_FILENO, &ch, 1) == 1) {
    if (ch == '\r' || ch == '\n') {
      std::cout << "\n" << std::flush;
      break;
    }
    if (ch == 127 || ch == 8) {
      if (!buf.empty()) {
        buf.pop_back();
        std::cout << "\b \b" << std::flush;
      }
    } else if (ch == 4) {
      return "--exit";
    } else if (ch >= 32 && ch <= 126) {
      buf.push_back(ch);
      std::cout << ansi::PHOSPHOR << ch << ansi::RESET << std::flush;
    }
  }
#else
  std::getline(std::cin, buf);
#endif
  return buf;
}

int main(int argc, char* argv[]) {
  std::signal(SIGINT, handle_signal);
  std::signal(SIGTERM, handle_signal);

  if (argc > 1) {
    std::string_view flag = argv[1];
    if (flag == "--rules" || flag == "--marla" || flag == "--soap" || flag == "--help") {
      dispatch(flag);
      return EXIT_SUCCESS;
    }
  }

  TerminalGuard guard;
  print_banner();
  crt_stream("Booting 1999 P31 Phosphor Terminal... Type --help or --soap to inspect factory.\n\n", ansi::PHOSPHOR_LO, 6, true);

  while (!g_interrupted.load()) {
    std::cout << ansi::BOLD << ansi::PHOSPHOR << "mayhem@paper-street:~$ " << ansi::RESET << std::flush;
    std::string input = guard.is_tty() ? read_raw_line() : ([&]() {
      std::string line;
      if (!std::getline(std::cin, line)) return std::string("--exit");
      return line;
    }());
    if (input.empty()) continue;
    if (!dispatch(input)) break;
  }
  return EXIT_SUCCESS;
}
