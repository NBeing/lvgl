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

1. How LVGL Works (General Overview)

LVGL is a graphics library that draws GUIs into a framebuffer (a memory buffer representing the screen).
LVGL is platform-agnostic: it doesn’t know or care about your actual hardware or OS.
To display graphics, LVGL needs you to provide:
A display driver (with a flush callback)
A buffer (where it draws pixels before you send them to the screen)

2. What the HAL Does
The HAL is your “glue” between LVGL and your actual hardware or OS.
It provides:
Functions to initialize the display/input
The flush callback (to send LVGL’s buffer to the real screen)
Input callbacks (for mouse/touch, etc.)
Timing/delay functions

3. Desktop (SDL) Mode

How It Works:

LVGL draws into a buffer (e.g., buf1), which is just a chunk of RAM.
When LVGL wants to update the screen, it calls your flush callback (sdl_display_flush).
In sdl_display_flush, you:
Copy the relevant part of the buffer (px_map) to an SDL texture using SDL_UpdateTexture.
Render the texture to the SDL window using SDL_RenderCopy and SDL_RenderPresent.
Result: The LVGL GUI appears in your SDL window.
Buffer Flow:

4. ESP32 Mode

How It Works:

LVGL draws into a buffer (e.g., buf1), just like on desktop.
When LVGL wants to update the screen, it calls your flush callback (e.g., my_disp_flush).
In your ESP32 flush callback, you:
Send the buffer (or just the changed area) to the display hardware (e.g., via SPI to an ILI9341 TFT).
When done, call lv_display_flush_ready() to tell LVGL it can reuse the buffer.
Result: The LVGL GUI appears on your physical display.

Buffer Flow:

5. Key Points
LVGL always draws to a buffer you provide.
The HAL’s flush callback is responsible for getting that buffer onto the real screen, whether that’s an SDL window or a hardware display.
On desktop, you use SDL to show the buffer in a window.
On ESP32, you use a display driver (like SPI) to send the buffer to the screen.

6. Why This Abstraction?

Portability: Your app code and GUI logic don’t care about the hardware.

Maintainability: You only need to change the HAL to support new hardware or platforms.

7. Summary Table
Platform	LVGL Draws To	Flush Callback	Sends Buffer To
Desktop	RAM buffer	sdl_display_flush	SDL texture/window
ESP32	RAM buffer	my_disp_flush	SPI/I2C display driver

In short:

LVGL draws to a buffer.
The HAL’s flush callback sends that buffer to the screen (SDL or hardware).
This makes your GUI code portable and hardware-independent!