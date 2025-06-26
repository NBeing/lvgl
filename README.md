# LVGL GUI Base project

This project is a cross-platform GUI built with [LVGL](https://lvgl.io/), supporting both desktop simulation (SDL2) and ESP32 hardware targets.  
It uses PlatformIO for build management and includes scripts for environment setup and dependency checks.

The test program is for a MIDI synth. This has been configured to work with my .i3 desktop environment so there may be some quirks on other DE's. 

---

## Quick Start

### 1. **Clone the Repository**

```sh
git clone https://github.com/NBeing/lvgl
cd lvgl
```

---

### 2. **Set Up Python Virtual Environment (Optional but Recommended)**

This project provides a script to create a Python virtual environment for running setup scripts and tools.

```sh
python3 scripts/setup_venv.py
# Follow the printed instructions to activate the venv
```

---

### 3. **Install SDL2 (for Desktop Simulation)**

The desktop simulator requires SDL2 development libraries.  
Run the provided script to check and install SDL2:

```sh
python3 scripts/install_sdl.py
```

- On Linux, this will attempt to install via your package manager.
- On Windows, it will check for vcpkg or guide you through manual installation.

You can also diagnose SDL2 issues with:

```sh
python3 scripts/dbg_sdl.py
```

---

### 4. **Configure PlatformIO**

All build environments are defined in `platformio.ini`.  
- **Desktop simulation:**  
  ```sh
  pio run -e desktop
  ```
- **ESP32 hardware:**  
  ```sh
  pio run -e rymcu-esp32-s3-devkitc-1
  ```

---

### 5. **Run the Desktop Simulator**

After building, run the simulator:

```sh
pio run -e desktop
pio run -t upload -e desktop  # Or use the VSCode task "PIO: Run Simulator"
```

Or directly:

```sh
./.pio/build/desktop/program
```

---

### 6. **Board Configuration (ESP32)**

To fetch the ESP32 board config used in this project:

```sh
bash scripts/get_board_conf.sh
```

---

## Scripts Overview

- **scripts/setup_venv.py**  
  Creates and configures a Python virtual environment for development scripts.

- **scripts/install_sdl.py**  
  Installs SDL2 development libraries for your platform.

- **scripts/dbg_sdl.py**  
  Diagnoses SDL2 installation issues and checks for headers, libraries, and pkg-config.

- **scripts/get_sdl_flags.py**  
  Outputs the correct SDL2 compiler flags for PlatformIO based on your system.

- **scripts/get_board_conf.sh**  
  Downloads the ESP32 board configuration JSON for PlatformIO.

---

## Troubleshooting

- If SDL2 is not detected, use `scripts/dbg_sdl.py` to diagnose.
- For missing Python dependencies, re-run `scripts/setup_venv.py`.
- For ESP32 upload/monitor, ensure you have the correct board connected and permissions set.

---

## Requirements

- [PlatformIO](https://platformio.org/) (VSCode extension recommended)
- Python 3.7+
- SDL2 development libraries (for desktop simulation)
- ESP32 toolchain (for hardware builds)

---

## Project Structure

- `src/` — Main application source code
- `include/` — LVGL and project headers
- `scripts/` — Setup and utility scripts
- `.vscode/` — VSCode workspace settings and tasks
- `platformio.ini` — PlatformIO environments and build flags

---
