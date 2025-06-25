#!/usr/bin/env python3
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
        "red": "\033[91m",
        "green": "\033[92m", 
        "yellow": "\033[93m",
        "blue": "\033[94m",
        "white": "\033[0m"
    }
    print(f"{colors.get(color, colors['white'])}{text}\033[0m")

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
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    printf("SDL2 is working correctly!\n");
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
    
    print_colored("\n=== Installation Complete ===", "blue")
    return success

if __name__ == "__main__":
    main()
