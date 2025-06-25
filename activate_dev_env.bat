@echo off
echo Activating LVGL development environment...

if not exist venv (
    echo Virtual environment not found. Running setup...
    python scripts\setup_venv.py
)

call venv\Scripts\activate.bat

echo.
echo Virtual environment activated!
echo Available commands:
echo   pio run -e desktop         : Build simulator
echo   pio run -e esp32s3         : Build for ESP32-S3
echo   python scripts\install_sdl.py : Install SDL2 dependencies
echo.
echo To deactivate, type: deactivate
echo.

cmd /k
