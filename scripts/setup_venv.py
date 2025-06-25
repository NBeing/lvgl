# scripts/setup_venv.py - Create and configure virtual environment
import os
import sys
import subprocess
import platform
from pathlib import Path

def run_command(cmd, shell=False):
    """Run a command and handle errors"""
    try:
        result = subprocess.run(cmd, shell=shell, check=True, capture_output=True, text=True)
        return result.stdout.strip()
    except subprocess.CalledProcessError as e:
        print(f"Error running command: {' '.join(cmd) if isinstance(cmd, list) else cmd}")
        print(f"Error: {e.stderr}")
        return None

def get_python_executable():
    """Get the correct Python executable (prefer python3 on Ubuntu)"""
    system = platform.system()
    
    # On Linux, prefer python3 explicitly
    if system == "Linux":
        # Try python3 first
        for cmd in ["python3", "python"]:
            try:
                result = subprocess.run([cmd, "--version"], capture_output=True, text=True)
                if result.returncode == 0 and "Python 3" in result.stdout:
                    print(f"Using Python executable: {cmd}")
                    return cmd
            except FileNotFoundError:
                continue
        
        print("Error: Python 3 not found. Please install python3:")
        print("sudo apt-get update && sudo apt-get install python3 python3-venv python3-pip")
        return None
    else:
        # On Windows/macOS, use sys.executable
        return sys.executable

def create_virtual_environment():
    """Create a virtual environment for the project"""
    venv_path = Path("venv")
    
    if venv_path.exists():
        print("Virtual environment already exists at ./venv")
        return venv_path
    
    print("Creating virtual environment...")
    
    # Get the correct Python executable
    python_cmd = get_python_executable()
    if not python_cmd:
        return None
    
    # Create virtual environment
    result = run_command([python_cmd, "-m", "venv", "venv"])
    if result is None:
        print("Failed to create virtual environment")
        print("If you get 'venv module not found', install it with:")
        print("sudo apt-get install python3-venv")
        return None
    
    print("Virtual environment created successfully!")
    return venv_path

def get_activation_command():
    """Get the correct activation command for the current platform"""
    system = platform.system()
    
    if system == "Windows":
        return "venv\\Scripts\\activate.bat"
    else:  # Linux/macOS
        return "source venv/bin/activate"

def install_python_dependencies(venv_path):
    """Install Python dependencies in the virtual environment"""
    system = platform.system()
    
    # Get the correct python executable path
    if system == "Windows":
        python_exe = venv_path / "Scripts" / "python.exe"
        pip_exe = venv_path / "Scripts" / "pip.exe"
    else:
        python_exe = venv_path / "bin" / "python"
        pip_exe = venv_path / "bin" / "pip"
    
    print("Installing Python dependencies...")
    
    # Upgrade pip first
    run_command([str(pip_exe), "install", "--upgrade", "pip"])
    
    # Install required packages
    packages = [
        "platformio",
        "requests",
        "psutil",
        "colorama",  # For colored output
        "click",     # For CLI tools
    ]
    
    for package in packages:
        print(f"Installing {package}...")
        result = run_command([str(pip_exe), "install", package])
        if result is None:
            print(f"Failed to install {package}")
            return False
    
    print("Python dependencies installed successfully!")
    return True

def create_requirements_txt():
    """Create requirements.txt file"""
    requirements = """# Python dependencies for LVGL ESP32 development
platformio>=6.1.0
requests>=2.28.0
psutil>=5.9.0
colorama>=0.4.6
click>=8.1.0

# Optional: For advanced features
# pygame>=2.1.0      # Alternative to SDL2 for simulator
# numpy>=1.24.0      # For signal processing
# matplotlib>=3.6.0  # For data visualization
"""
    
    with open("requirements.txt", "w") as f:
        f.write(requirements)
    
    print("Created requirements.txt")

def create_activation_scripts():
    """Create platform-specific activation scripts"""
    
    # Windows batch script
    windows_script = """@echo off
echo Activating LVGL development environment...

if not exist venv (
    echo Virtual environment not found. Running setup...
    python scripts\\setup_venv.py
)

call venv\\Scripts\\activate.bat

echo.
echo Virtual environment activated!
echo Available commands:
echo   pio run -e desktop         : Build simulator
echo   pio run -e esp32s3         : Build for ESP32-S3
echo   python scripts\\install_sdl.py : Install SDL2 dependencies
echo.
echo To deactivate, type: deactivate
echo.

cmd /k
"""
    
    # Unix shell script (Linux/macOS) - Updated for python3
    unix_script = """#!/bin/bash

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
"""
    
    # Write Windows script
    with open("activate_dev_env.bat", "w") as f:
        f.write(windows_script)
    
    # Write Unix script
    with open("activate_dev_env.sh", "w") as f:
        f.write(unix_script)
    
    # Make Unix script executable
    if platform.system() != "Windows":
        os.chmod("activate_dev_env.sh", 0o755)
    
    print("Created activation scripts:")
    print("  Windows: activate_dev_env.bat")
    print("  Linux/macOS: activate_dev_env.sh")

def update_install_sdl_script():
    """Update the SDL installation script to work better in venv"""
    sdl_script = '''#!/usr/bin/env python3
"""
SDL2 Installation Script for LVGL Development
Installs SDL2 development libraries for the simulator environment
"""

import platform
import subprocess
import sys
import os
import urllib.request
import zipfile
import shutil
from pathlib import Path

def print_colored(text, color="white"):
    """Print colored text to terminal"""
    colors = {
        "red": "\\033[91m",
        "green": "\\033[92m", 
        "yellow": "\\033[93m",
        "blue": "\\033[94m",
        "white": "\\033[0m"
    }
    print(f"{colors.get(color, colors['white'])}{text}\\033[0m")

def run_command(cmd, shell=False):
    """Run a command safely"""
    try:
        result = subprocess.run(cmd, shell=shell, check=True, capture_output=True, text=True)
        return True, result.stdout
    except subprocess.CalledProcessError as e:
        return False, e.stderr

def install_sdl2_windows():
    """Install SDL2 on Windows"""
    print_colored("Installing SDL2 for Windows...", "blue")
    
    # Try vcpkg first
    success, output = run_command(["vcpkg", "list", "sdl2"], shell=True)
    if success and "sdl2" in output:
        print_colored("SDL2 already installed via vcpkg!", "green")
        return True
    
    # Try to install via vcpkg
    print("Trying to install SDL2 via vcpkg...")
    success, output = run_command(["vcpkg", "install", "sdl2:x64-windows"], shell=True)
    if success:
        print_colored("SDL2 installed successfully via vcpkg!", "green")
        return True
    
    # Manual installation guide
    print_colored("Vcpkg not available. Manual installation required:", "yellow")
    print("1. Download SDL2 development libraries from:")
    print("   https://www.libsdl.org/download-2.0.php")
    print("2. Extract to C:/SDL2/ or C:/vcpkg/installed/x64-windows/")
    print("3. Or install vcpkg and run: vcpkg install sdl2:x64-windows")
    
    return False

def install_sdl2_linux():
    """Install SDL2 on Linux"""
    print_colored("Installing SDL2 for Linux...", "blue")
    
    # Try different package managers
    package_managers = [
        (["sudo", "apt-get", "update"], ["sudo", "apt-get", "install", "-y", "libsdl2-dev"]),
        (["sudo", "dnf", "install", "-y", "SDL2-devel"], None),
        (["sudo", "yum", "install", "-y", "SDL2-devel"], None),
        (["sudo", "pacman", "-S", "--noconfirm", "sdl2"], None),
        (["sudo", "zypper", "install", "-y", "libSDL2-devel"], None)
    ]
    
    for update_cmd, install_cmd in package_managers:
        if update_cmd:
            print(f"Trying: {' '.join(update_cmd)}")
            run_command(update_cmd)
        
        cmd = install_cmd if install_cmd else update_cmd
        print(f"Trying: {' '.join(cmd)}")
        success, output = run_command(cmd)
        
        if success:
            print_colored("SDL2 installed successfully!", "green")
            return True
    
    print_colored("Could not install SDL2 automatically.", "red")
    print("Please install manually:")
    print("  Ubuntu/Debian: sudo apt-get install libsdl2-dev")
    print("  Fedora: sudo dnf install SDL2-devel")
    print("  Arch: sudo pacman -S sdl2")
    
    return False

def install_sdl2_macos():
    """Install SDL2 on macOS"""
    print_colored("Installing SDL2 for macOS...", "blue")
    
    # Try Homebrew
    success, output = run_command(["brew", "--version"])
    if success:
        print("Installing SDL2 via Homebrew...")
        success, output = run_command(["brew", "install", "sdl2"])
        if success:
            print_colored("SDL2 installed successfully via Homebrew!", "green")
            return True
    
    # Try MacPorts
    success, output = run_command(["port", "version"])
    if success:
        print("Installing SDL2 via MacPorts...")
        success, output = run_command(["sudo", "port", "install", "libsdl2"])
        if success:
            print_colored("SDL2 installed successfully via MacPorts!", "green")
            return True
    
    print_colored("Could not install SDL2 automatically.", "red")
    print("Please install Homebrew (https://brew.sh/) and run:")
    print("  brew install sdl2")
    
    return False

def verify_sdl2_installation():
    """Verify that SDL2 is properly installed"""
    print_colored("Verifying SDL2 installation...", "blue")
    
    # Try to compile a simple test program
    test_program = """#include <SDL2/SDL.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\\n", SDL_GetError());
        return 1;
    }
    printf("SDL2 is working correctly!\\n");
    SDL_Quit();
    return 0;
}"""
    
    # Write test file
    with open("sdl_test.c", "w") as f:
        f.write(test_program)
    
    # Try to compile
    compile_commands = [
        ["gcc", "sdl_test.c", "-lSDL2", "-o", "sdl_test"],
        ["gcc", "sdl_test.c", "-I/usr/include/SDL2", "-lSDL2", "-o", "sdl_test"],
        ["gcc", "sdl_test.c", "`pkg-config --cflags --libs sdl2`", "-o", "sdl_test"]
    ]
    
    for cmd in compile_commands:
        success, output = run_command(cmd, shell=True)
        if success:
            # Try to run the test
            success, output = run_command(["./sdl_test"], shell=True)
            if success and "working correctly" in output:
                print_colored("SDL2 verification successful!", "green")
                # Clean up
                for file in ["sdl_test.c", "sdl_test", "sdl_test.exe"]:
                    if os.path.exists(file):
                        os.remove(file)
                return True
    
    # Clean up test files
    for file in ["sdl_test.c", "sdl_test", "sdl_test.exe"]:
        if os.path.exists(file):
            os.remove(file)
    
    print_colored("SDL2 verification failed.", "yellow")
    print("SDL2 might be installed but not properly configured.")
    return False

def main():
    """Main installation function"""
    print_colored("=== SDL2 Installation for LVGL Development ===", "blue")
    
    system = platform.system()
    success = False
    
    if system == "Windows":
        success = install_sdl2_windows()
    elif system == "Linux":
        success = install_sdl2_linux()
    elif system == "Darwin":
        success = install_sdl2_macos()
    else:
        print_colored(f"Unsupported platform: {system}", "red")
        return False
    
    if success:
        verify_sdl2_installation()
    
    print_colored("\\n=== Installation Complete ===", "blue")
    return success

if __name__ == "__main__":
    main()
'''
    
    with open("scripts/install_sdl.py", "w") as f:
        f.write(sdl_script)
    
    # Make executable on Unix systems
    if platform.system() != "Windows":
        os.chmod("scripts/install_sdl.py", 0o755)
    
    print("Updated scripts/install_sdl.py with virtual environment support")

def main():
    """Main setup function"""
    print("=== LVGL ESP32 Development Environment Setup ===")
    print()
    
    # Check for python3 on Linux
    if platform.system() == "Linux":
        try:
            result = subprocess.run(["python3", "--version"], capture_output=True, text=True)
            if result.returncode != 0:
                print("Error: python3 not found!")
                print("Please install python3 and required packages:")
                print("sudo apt-get update")
                print("sudo apt-get install python3 python3-venv python3-pip")
                return False
            else:
                print(f"Found: {result.stdout.strip()}")
        except FileNotFoundError:
            print("Error: python3 not found!")
            print("Please install python3:")
            print("sudo apt-get update && sudo apt-get install python3 python3-venv python3-pip")
            return False
    
    # Create virtual environment
    venv_path = create_virtual_environment()
    if not venv_path:
        return False
    
    # Install Python dependencies
    if not install_python_dependencies(venv_path):
        return False
    
    # Create project files
    create_requirements_txt()
    create_activation_scripts()
    update_install_sdl_script()
    
    print()
    print("=== Setup Complete! ===")
    print()
    print("To activate your development environment:")
    
    system = platform.system()
    if system == "Windows":
        print("  Double-click: activate_dev_env.bat")
        print("  Or run: activate_dev_env.bat")
    else:
        print("  Run: ./activate_dev_env.sh")
        print("  Or: source activate_dev_env.sh")
    
    print()
    print("Once activated, you can:")
    print("  - Install SDL2: python scripts/install_sdl.py")
    print("  - Build simulator: pio run -e desktop")
    print("  - Build ESP32: pio run -e esp32s3")
    
    return True

if __name__ == "__main__":
    main()
