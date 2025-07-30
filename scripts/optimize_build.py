#!/usr/bin/env python3
"""
Build optimization script for PlatformIO
Enables parallel compilation and other optimizations
"""

import subprocess
import multiprocessing
import os

def get_cpu_count():
    """Get the number of CPU cores for parallel compilation"""
    return multiprocessing.cpu_count()

def main():
    """Configure build optimizations"""
    cpu_count = get_cpu_count()
    
    print(f"🚀 Build Optimization: Detected {cpu_count} CPU cores")
    
    # Set parallel build environment variables
    os.environ['MAKEFLAGS'] = f'-j{cpu_count}'
    os.environ['CMAKE_BUILD_PARALLEL_LEVEL'] = str(cpu_count)
    
    # Enable ccache if available (speeds up repeated compilations)
    if subprocess.run(['which', 'ccache'], capture_output=True).returncode == 0:
        print("✅ ccache detected - enabling compiler caching")
        os.environ['CC'] = 'ccache gcc'
        os.environ['CXX'] = 'ccache g++'
    else:
        print("ℹ️  ccache not found - install with: sudo apt install ccache (for even faster builds)")
    
    print(f"🔧 Parallel compilation enabled with {cpu_count} jobs")

if __name__ == "__main__":
    main()
