#!/usr/bin/env python3
"""
Serial MIDI Bridge - Connect ESP32-S3 serial MIDI to ALSA MIDI
This script reads MIDI data from ESP32-S3 serial port and sends it to FluidSynth
"""

import serial
import time
import subprocess
import sys
import signal
import re

def find_esp32_port():
    """Find ESP32-S3 serial port"""
    import glob
    
    # Common ESP32-S3 serial ports
    possible_ports = ['/dev/ttyACM1', '/dev/ttyACM0', '/dev/ttyUSB0', '/dev/ttyUSB1']
    
    for port in possible_ports:
        try:
            if glob.glob(port):
                print(f"🔍 Found potential ESP32-S3 port: {port}")
                return port
        except:
            continue
    
    return None

def find_fluidsynth_port():
    """Find FluidSynth ALSA MIDI port"""
    try:
        result = subprocess.run(['aconnect', '-l'], capture_output=True, text=True)
        lines = result.stdout.split('\n')
        
        for line in lines:
            if 'FLUID Synth' in line:
                # Extract port number
                match = re.search(r'client (\d+):', line)
                if match:
                    return match.group(1)
    except:
        pass
    
    return None

def parse_midi_from_serial(line):
    """Extract MIDI data from ESP32-S3 serial output"""
    # Look for MIDI messages in the serial output
    # Example: "ESP32 USB MIDI NoteOn: Ch2 Note64 Vel80"
    
    if "MIDI NoteOn:" in line:
        # Parse Note On
        match = re.search(r'Ch(\d+) Note(\d+) Vel(\d+)', line)
        if match:
            channel = int(match.group(1)) - 1  # Convert to 0-based
            note = int(match.group(2))
            velocity = int(match.group(3))
            # MIDI Note On: 0x90 + channel, note, velocity
            return bytes([0x90 + channel, note, velocity])
    
    elif "MIDI NoteOff:" in line:
        # Parse Note Off
        match = re.search(r'Ch(\d+) Note(\d+) Vel(\d+)', line)
        if match:
            channel = int(match.group(1)) - 1  # Convert to 0-based
            note = int(match.group(2))
            velocity = int(match.group(3))
            # MIDI Note Off: 0x80 + channel, note, velocity
            return bytes([0x80 + channel, note, velocity])
    
    return None

def send_midi_to_alsa(midi_bytes, fluid_port):
    """Send MIDI bytes to FluidSynth via amidi"""
    try:
        # Create temporary MIDI file
        with open('/tmp/midi_note.mid', 'wb') as f:
            # Write minimal MIDI file header + our MIDI data
            header = bytes([
                0x4D, 0x54, 0x68, 0x64,  # "MThd"
                0x00, 0x00, 0x00, 0x06,  # Header length
                0x00, 0x00,              # Format 0
                0x00, 0x01,              # 1 track
                0x00, 0x60,              # 96 ticks per quarter note
                0x4D, 0x54, 0x72, 0x6B,  # "MTrk"
                0x00, 0x00, 0x00, len(midi_bytes) + 1,  # Track length
                0x00                     # Delta time
            ])
            f.write(header + midi_bytes)
        
        # Use aplaymidi to send to FluidSynth
        subprocess.run(['aplaymidi', '-p', f'{fluid_port}:0', '/tmp/midi_note.mid'], 
                      capture_output=True)
        return True
    except Exception as e:
        print(f"❌ Error sending MIDI: {e}")
        return False

def main():
    print("🎵 ESP32-S3 Serial MIDI Bridge")
    print("   Connecting ESP32-S3 serial MIDI to FluidSynth...")
    
    # Find ports
    esp32_port = find_esp32_port()
    fluid_port = find_fluidsynth_port()
    
    if not esp32_port:
        print("❌ No ESP32-S3 serial port found!")
        print("   Make sure ESP32-S3 is connected via USB")
        return 1
    
    if not fluid_port:
        print("❌ FluidSynth not found!")
        print("   Start FluidSynth first: fluidsynth -a alsa soundfont.sf2")
        return 1
    
    print(f"✅ ESP32-S3 port: {esp32_port}")
    print(f"✅ FluidSynth port: {fluid_port}")
    
    # Set up signal handler for clean exit
    def signal_handler(sig, frame):
        print("\n🛑 Stopping MIDI bridge...")
        sys.exit(0)
    
    signal.signal(signal.SIGINT, signal_handler)
    
    try:
        # Open serial connection to ESP32-S3
        print(f"🔌 Connecting to ESP32-S3 on {esp32_port}...")
        ser = serial.Serial(esp32_port, 115200, timeout=0.1)
        
        print("🎵 MIDI Bridge active! Press Ctrl+C to stop")
        print("   Listening for MIDI from ESP32-S3...")
        
        while True:
            try:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    # Parse MIDI from serial output
                    midi_bytes = parse_midi_from_serial(line)
                    if midi_bytes:
                        print(f"🎵 MIDI: {line}")
                        success = send_midi_to_alsa(midi_bytes, fluid_port)
                        if success:
                            print(f"   ✅ Sent to FluidSynth: {midi_bytes.hex()}")
                        else:
                            print(f"   ❌ Failed to send MIDI")
                
                time.sleep(0.001)  # Small delay to prevent CPU overload
                
            except serial.SerialException as e:
                print(f"❌ Serial error: {e}")
                break
            except KeyboardInterrupt:
                break
    
    except Exception as e:
        print(f"❌ Error: {e}")
        return 1
    finally:
        if 'ser' in locals():
            ser.close()
    
    print("🛑 MIDI bridge stopped")
    return 0

if __name__ == "__main__":
    sys.exit(main())
