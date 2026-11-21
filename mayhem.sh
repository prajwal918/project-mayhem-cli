#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
SRC_FILE="${SCRIPT_DIR}/src/project_mayhem.cpp"
BIN_FILE="${SCRIPT_DIR}/build/project_mayhem"

cleanup_tty() {
  printf '\033[0m\033[?25h'
  if [[ -t 0 ]]; then
    stty sane 2>/dev/null || true
  fi
}
trap cleanup_tty EXIT INT TERM

mkdir -p "${SCRIPT_DIR}/build"

if [[ ! -x "${BIN_FILE}" || "${SRC_FILE}" -nt "${BIN_FILE}" ]]; then
  CXX_COMPILER="$(command -v g++ || command -v clang++ || true)"
  if [[ -z "${CXX_COMPILER}" ]]; then
    printf '\033[38;2;255;20;147m[ERROR]\033[0m C++20 compiler (g++ or clang++) not found.\n' >&2
    exit 1
  fi
  printf '\033[38;2;0;255;102m[BUILD]\033[0m Compiling PROJECT-MAYHEM-CLI with %s (-std=c++20 -O2)...\n' "${CXX_COMPILER}"
  "${CXX_COMPILER}" -std=c++20 -O2 -Wall -Wextra -o "${BIN_FILE}" "${SRC_FILE}"
fi

if [[ -t 1 ]]; then
  printf '\033]0;PAPER STREET SOAP CO. // PROJECT-MAYHEM CRT\007'
fi

exec "${BIN_FILE}" "$@"
