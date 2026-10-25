#!/usr/bin/env bash
# huEVO Xbox 360 Bridge Launcher for Arch Linux / CachyOS / Manjaro / Debian / Ubuntu

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Check for Python 3
if ! command -v python3 &>/dev/null; then
    echo "[-] Python 3 не найден в системе!"
    echo "    Установите его командой: sudo pacman -S python"
    read -p "Нажмите Enter для выхода..."
    exit 1
fi

echo "=========================================================="
echo "  huEVO Xbox 360 Companion Bridge (Arch Linux / CachyOS)"
echo "=========================================================="
echo "[*] Запуск локального моста для Xbox 360..."
echo ""

python3 bridge.py
