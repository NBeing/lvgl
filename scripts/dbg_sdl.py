#!/usr/bin/env python3
"""
SDL2 Debug Script - Diagnose SDL2 installation issues
"""

import subprocess
import os
import sys

def run_command(cmd, shell=False):
    """Run a command and return success, stdout, stderr"""
    try:
        result = subprocess.run(cmd, shell=shell, capture_output=True, text=True)
        return result.returncode == 0, result.stdout, result.stderr
    except Exception as e:
        return False, "", str(e)

def check_sdl2_packages():
    """Check if SDL2 packages are installed"""
    print("=== Checking SDL2 Package Installation ===")
    
    packages = ["libsdl2-dev", "libsdl2-2.0-0", "pkg-config"]
    
    for package in packages:
        success, stdout, stderr = run_command(["dpkg", "-l", package])
        if success and package in stdout:
            print(f"✓ {package} is installed")
        else:
            print(f"✗ {package} is NOT installed")
            print(f"  Install with: sudo apt-get install {package}")

def check_sdl2_headers():
    """Check if SDL2 header files exist"""
    print("\n=== Checking SDL2 Header Files ===")
    
    header_paths = [
        "/usr/include/SDL2/SDL.h",
        "/usr/local/include/SDL2/SDL.h"
    ]
    
    found = False
    for path in header_paths:
        if os.path.exists(path):
            print(f"✓ Found SDL2 headers at: {path}")
            found = True
        else:
            print(f"✗ Not found: {path}")
    
    if not found:
        print("No SDL2 headers found. Install with: sudo apt-get install libsdl2-dev")

def check_sdl2_libraries():
    """Check if SDL2 libraries exist"""
    print("\n=== Checking SDL2 Libraries ===")
    
    lib_paths = [
        "/usr/lib/x86_64-linux-gnu/libSDL2.so",
        "/usr/lib/libSDL2.so",
        "/usr/local/lib/libSDL2.so"
    ]
    
    found = False
    for path in lib_paths:
        if os.path.exists(path):
            print(f"✓ Found SDL2 library at: {path}")
            found = True
        else:
            print(f"✗ Not found: {path}")
    
    if not found:
        print("No SDL2 libraries found. Install with: sudo apt-get install libsdl2-2.0-0")

def check_pkg_config():
    """Check pkg-config for SDL2"""
    print("\n=== Checking pkg-config for SDL2 ===")
    
    success, stdout, stderr = run_command(["pkg-config", "--exists", "sdl2"])
    if success:
        print("✓ pkg-config finds SDL2")
        
        # Get cflags
        success, cflags, stderr = run_command(["pkg-config", "--cflags", "sdl2"])
        if success:
            print(f"  CFLAGS: {cflags.strip()}")
        
        # Get libs
        success, libs, stderr = run_command(["pkg-config", "--libs", "sdl2"])
        if success:
            print(f"  LIBS: {libs.strip()}")
    else:
        print("✗ pkg-config cannot find SDL2")
        print("  Install pkg-config: sudo apt-get install pkg-config")

def test_simple_compile():
    """Test a simple SDL2 compilation"""
    print("\n=== Testing SDL2 Compilation ===")
    
    # Create a simple test program
    test_program = """#include <SDL2/SDL.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    printf("Testing SDL2...\\n");
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init failed: %s\\n", SDL_GetError());
        return 1;
    }
    
    printf("SDL2 initialized successfully!\\n");
    SDL_Quit();
    return 0;
}"""
    
    # Write test file
    with open("sdl_test_debug.c", "w") as f:
        f.write(test_program)
    
    print("Created test file: sdl_test_debug.c")
    
    # Try different compilation approaches
    compile_commands = [
        # Using pkg-config (preferred)
        "gcc sdl_test_debug.c $(pkg-config --cflags --libs sdl2) -o sdl_test_debug",
        
        # Manual include/lib paths
        "gcc sdl_test_debug.c -I/usr/include/SDL2 -lSDL2 -o sdl_test_debug",
        
        # Alternative library name
        "gcc sdl_test_debug.c -I/usr/include/SDL2 -lSDL2main -lSDL2 -o sdl_test_debug"
    ]
    
    for i, cmd in enumerate(compile_commands, 1):
        print(f"\nTrying compilation method {i}:")
        print(f"Command: {cmd}")
        
        success, stdout, stderr = run_command(cmd, shell=True)
        
        if success:
            print("✓ Compilation successful!")
            
            # Try to run the test
            if os.path.exists("sdl_test_debug"):
                print("Running test program...")
                success, stdout, stderr = run_command(["./sdl_test_debug"])
                
                if success:
                    print("✓ Test program ran successfully!")
                    print(f"Output: {stdout}")
                    
                    # Clean up and return success
                    cleanup_test_files()
                    return True
                else:
                    print("✗ Test program failed to run")
                    print(f"Error: {stderr}")
            break
        else:
            print("✗ Compilation failed")
            print(f"Stdout: {stdout}")
            print(f"Stderr: {stderr}")
    
    cleanup_test_files()
    return False

def cleanup_test_files():
    """Remove test files"""
    for file in ["sdl_test_debug.c", "sdl_test_debug"]:
        if os.path.exists(file):
            os.remove(file)

def check_display_environment():
    """Check if we have a display for testing"""
    print("\n=== Checking Display Environment ===")
    
    display = os.environ.get("DISPLAY")
    if display:
        print(f"✓ DISPLAY environment variable set: {display}")
    else:
        print("✗ No DISPLAY environment variable")
        print("  This might cause SDL2 to fail in headless environments")
        print("  For headless testing, you might need Xvfb")

def main():
    """Run all diagnostic tests"""
    print("SDL2 Installation Diagnostic Tool")
    print("=" * 40)
    
    check_sdl2_packages()
    check_sdl2_headers()
    check_sdl2_libraries()
    check_pkg_config()
    check_display_environment()
    
    success = test_simple_compile()
    
    print("\n" + "=" * 40)
    if success:
        print("✓ SDL2 is working correctly!")
        print("The issue might be with the original verification script.")
    else:
        print("✗ SDL2 has configuration issues")
        print("\nSuggested fixes:")
        print("1. Install missing packages:")
        print("   sudo apt-get update")
        print("   sudo apt-get install libsdl2-dev libsdl2-2.0-0 pkg-config build-essential")
        print("2. If you're using SSH, enable X11 forwarding:")
        print("   ssh -X username@hostname")
        print("3. For headless systems, install Xvfb:")
        print("   sudo apt-get install xvfb")
        print("   Run with: xvfb-run -a ./your_program")

if __name__ == "__main__":
    main()