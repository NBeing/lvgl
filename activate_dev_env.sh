#!/bin/bash

echo "Activating LVGL development environment..."

# Check if python3 is available
if ! command -v python3 &> /dev/null; then
    echo "Error: python3 not found. Please install it:"
    echo "sudo apt-get update && sudo apt-get install python3 python3-venv python3-pip"
    exit 1
fi

# Check if venv module is available
if ! python3 -c "import venv" &> /dev/null; then
    echo "Error: python3-venv not found. Installing..."
    sudo apt-get update && sudo apt-get install python3-venv
fi

if [ ! -d "venv" ]; then
    echo "Virtual environment not found. Running setup..."
    python3 scripts/setup_venv.py
fi

source venv/bin/activate

echo ""
echo "Virtual environment activated!"
echo "Python version: $(python --version)"
echo "Pip version: $(pip --version)"
echo ""
echo "Available commands:"
echo "  pio run -e desktop         : Build simulator"
echo "  pio run -e esp32s3         : Build for ESP32-S3"
echo "  python scripts/install_sdl.py : Install SDL2 dependencies"
echo ""
echo "To deactivate, type: deactivate"
echo ""

# Keep shell open
exec "$SHELL"
