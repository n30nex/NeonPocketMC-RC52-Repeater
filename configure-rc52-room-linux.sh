#!/bin/sh
set -eu
cd "$(dirname "$0")"

if ! command -v python3 >/dev/null 2>&1; then
  echo "Python 3 is required. Install python3 and python3-venv, then try again." >&2
  exit 1
fi

if [ ! -x .neonpocket-venv/bin/python ]; then
  echo "Preparing the one-time setup helper..."
  python3 -m venv .neonpocket-venv
fi

.neonpocket-venv/bin/python -m pip install --disable-pip-version-check -q pyserial
exec .neonpocket-venv/bin/python scripts/configure_rc52_room.py "$@"
