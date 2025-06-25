#!/usr/bin/env python3
"""
get_sdl_flags.py - Generate SDL2 compiler flags for PlatformIO
This script detects the platform and outputs the correct SDL2 compiler flags
"""

import platform
import subprocess
import os
import sys

def get_sdl_flags():
    """Generate SDL2 compiler flags for the current platform"""
    system = platform.system()
    flags = []
    
    if system == "Windows":
        # Common Windows SDL2 paths
        possible_paths = [
            "C:/SDL2",
            "C:/vcpkg/installed/x64-windows",
            "C:/vcpkg/installed/x86-windows", 
            "C:/msys64/mingw64",
            "/mingw64",
            "C:/MinGW/msys/1.0"
        ]
        
        # Try to find SDL2 installation
        for path in possible_paths:
            include_path = f"{path}/include/SDL2"
            lib_path = f"{path}/lib"
            
            if os.path.exists(include_path) and os.path.exists(lib_path):
                flags.extend([
                    f"-I{include_path}",
                    f"-L{lib_path}",
                    "-lSDL2main",
                    "-lSDL2"
                ])
                break
        else:
            # Fallback flags for Windows
            flags.extend([
                "-lSDL2main", 
                "-lSDL2",
                "-Dmain=SDL_main"  # SDL2 main wrapper
            ])
            
    elif system == "Linux":
        try:
            # Use pkg-config to get proper flags (preferred method)
            result = subprocess.run(
                ["pkg-config", "--cflags", "--libs", "sdl2"], 
                capture_output=True, 
                text=True, 
                check=True
            )
            flags.extend(result.stdout.strip().split())
            
        except (subprocess.CalledProcessError, FileNotFoundError):
            # Fallback for Linux if pkg-config fails
            flags.extend([
                "-I/usr/include/SDL2",
                "-D_REENTRANT",
                "-lSDL2"
            ])
            
    elif system == "Darwin":  # macOS
        try:
            # Try pkg-config first
            result = subprocess.run(
                ["pkg-config", "--cflags", "--libs", "sdl2"], 
                capture_output=True, 
                text=True, 
                check=True
            )
            flags.extend(result.stdout.strip().split())
            
        except (subprocess.CalledProcessError, FileNotFoundError):
            try:
                # Homebrew fallback
                homebrew_result = subprocess.run(
                    ["brew", "--prefix"], 
                    capture_output=True, 
                    text=True,
                    check=True
                )
                homebrew_prefix = homebrew_result.stdout.strip()
                
                flags.extend([
                    f"-I{homebrew_prefix}/include/SDL2",
                    f"-L{homebrew_prefix}/lib",
                    "-lSDL2"
                ])
                
            except (subprocess.CalledProcessError, FileNotFoundError):
                # MacPorts fallback
                flags.extend([
                    "-I/opt/local/include/SDL2",
                    "-L/opt/local/lib", 
                    "-lSDL2"
                ])
    
    else:
        # Unknown platform fallback
        flags.extend(["-lSDL2"])
    
    # Print flags for PlatformIO to use
    print(" ".join(flags))
    return flags

def verify_flags(flags):
    """Verify that the generated flags work"""
    if not flags:
        return False
        
    # Create a simple test program
    test_code = '''
#include <SDL2/SDL.h>
int main(int argc, char* argv[]) {
    SDL_Init(0);
    SDL_Quit();
    return 0;
}
'''
    
    try:
        with open("test_sdl_flags.c", "w") as f:
            f.write(test_code)
        
        # Try to compile with the generated flags
        compile_cmd = ["gcc", "test_sdl_flags.c"] + flags + ["-o", "test_sdl_flags"]
        
        result = subprocess.run(
            compile_cmd,
            capture_output=True,
            text=True
        )
        
        success = result.returncode == 0
        
        # Clean up
        for file in ["test_sdl_flags.c", "test_sdl_flags", "test_sdl_flags.exe"]:
            if os.path.exists(file):
                os.remove(file)
                
        return success
        
    except Exception:
        return False

def main():
    """Main function - can be used standalone or imported"""
    if len(sys.argv) > 1 and sys.argv[1] == "--verify":
        # Verification mode
        flags = get_sdl_flags()
        if verify_flags(flags):
            print("SDL2 flags verified successfully!", file=sys.stderr)
            return 0
        else:
            print("SDL2 flags verification failed!", file=sys.stderr)
            return 1
    else:
        # Normal mode - just output flags
        get_sdl_flags()
        return 0

if __name__ == "__main__":
    sys.exit(main())