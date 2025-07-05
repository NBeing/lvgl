import os
import shutil
import subprocess
import sys
from pathlib import Path

# Try to import PlatformIO environment, but don't fail if not available
try:
    Import("env")
    PLATFORMIO_ENV = True
except:
    PLATFORMIO_ENV = False

# Configuration
RTMIDI_VERSION = "6.0.0"
RTMIDI_URL = f"https://github.com/thestk/rtmidi/archive/refs/tags/{RTMIDI_VERSION}.tar.gz"
PROJECT_DIR = Path(os.getcwd())
LIB_DIR = PROJECT_DIR / "lib" / "rtmidi_linux"
REQUIRED_FILES = ["RtMidi.h", "RtMidi.cpp"]

def download_and_extract_rtmidi():
    """Download and extract only the files we need from RtMidi"""
    print(f"📦 Installing RtMidi {RTMIDI_VERSION} for Linux...")
    
    # Create lib directory
    LIB_DIR.mkdir(parents=True, exist_ok=True)
    
    # Download tarball
    import urllib.request
    import tarfile
    import tempfile
    
    with tempfile.TemporaryDirectory() as temp_dir:
        tarball_path = Path(temp_dir) / f"rtmidi-{RTMIDI_VERSION}.tar.gz"
        
        print(f"📥 Downloading RtMidi {RTMIDI_VERSION}...")
        urllib.request.urlretrieve(RTMIDI_URL, tarball_path)
        
        # Extract only the files we need
        print("📂 Extracting required files...")
        with tarfile.open(tarball_path, "r:gz") as tar:
            for file in REQUIRED_FILES:
                try:
                    # Extract from the root of the archive
                    member = tar.getmember(f"rtmidi-{RTMIDI_VERSION}/{file}")
                    member.name = file  # Flatten the path
                    tar.extract(member, LIB_DIR)
                    print(f"✅ Extracted {file}")
                except KeyError:
                    print(f"❌ Could not find {file} in archive")
                    return False
    
    # Create library.json
    create_library_json()
    
    print("✅ RtMidi installation complete!")
    return True

def create_library_json():
    """Create a proper library.json for our RtMidi installation"""
    library_json = {
        "name": "rtmidi_linux",
        "version": RTMIDI_VERSION,
        "description": "RtMidi Linux-only build (ALSA)",
        "keywords": ["midi", "alsa", "linux"],
        "authors": [
            {
                "name": "Gary P. Scavone",
                "email": "gary@music.mcgill.ca"
            }
        ],
        "repository": {
            "type": "git",
            "url": "https://github.com/thestk/rtmidi.git"
        },
        "license": "MIT",
        "build": {
            "flags": [
                "-D__LINUX_ALSA__",
                "-D__LITTLE_ENDIAN__",
                "-DHAVE_GETTIMEOFDAY"
            ],
            "srcFilter": [
                "+<RtMidi.cpp>"
            ]
        },
        "frameworks": ["*"],
        "platforms": ["native", "linux_x86_64"]
    }
    
    import json
    with open(LIB_DIR / "library.json", "w") as f:
        json.dump(library_json, f, indent=2)
    
    print("✅ Created library.json")

def check_rtmidi_installation():
    """Check if RtMidi is already properly installed"""
    if not LIB_DIR.exists():
        return False
    
    # Check if all required files exist
    for file in REQUIRED_FILES:
        if not (LIB_DIR / file).exists():
            print(f"❌ Missing required file: {file}")
            return False
    
    # Check if library.json exists
    if not (LIB_DIR / "library.json").exists():
        print("❌ Missing library.json")
        return False
    
    return True

def main():
    """Main installation function"""
    print(f"🔧 RtMidi installer - Working directory: {PROJECT_DIR}")
    
    if check_rtmidi_installation():
        print("⚠️  RtMidi is already installed. Skipping download.")
        print(f"📁 Location: {LIB_DIR.absolute()}")
        print("💡 To reinstall, delete the lib/rtmidi_linux directory")
        return
    
    # Install RtMidi
    success = download_and_extract_rtmidi()
    if not success:
        print("❌ RtMidi installation failed!")
        sys.exit(1)

# Run the installation
if __name__ == "__main__":
    main()
elif PLATFORMIO_ENV:
    # Called from PlatformIO - run before dependency resolution
    print("🔧 PlatformIO pre-build: Installing RtMidi...")
    main()