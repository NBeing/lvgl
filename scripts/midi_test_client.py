#!/usr/bin/env python3
"""
LVGL Synthesizer MIDI Test Client
=================================

Automated MIDI testing client that connects to the LVGL Synthesizer
and sends various test patterns to validate MIDI functionality.

Commands:
    clock on <bpm>     - Start MIDI clock at specified BPM
    clock stop         - Stop MIDI clock
    cc <controller> <value>  - Send CC message
    note <note> <velocity>   - Send note on/off
    test basic         - Run basic parameter tests
    test stress        - Run stress tests
    quit               - Exit the client
"""

import rtmidi
import time
import sys
import threading
import argparse
from typing import Optional, List


class MidiTestClient:
    """Automated MIDI test client for LVGL Synthesizer"""
    
    def __init__(self, verbose=False):
        """Initialize the MIDI test client"""
        self.midi_out = rtmidi.MidiOut()
        self.midi_in = rtmidi.MidiIn()
        self.synth_input_port = None
        self.synth_output_port = None
        self.running = False
        self.verbose = verbose
        
        # MIDI Clock state
        self.clock_running = False
        self.clock_thread = None
        self.clock_bpm = 120
        self.clock_ppqn = 24  # Pulses per quarter note (MIDI standard)
        
        # MIDI Real-Time messages
        self.MIDI_CLOCK = 0xF8
        self.MIDI_START = 0xFA
        self.MIDI_CONTINUE = 0xFB
        self.MIDI_STOP = 0xFC
        
        # Test sequences
        self.cc_test_sequence = [
            # (CC#, Value, Parameter Name)
            (74, 64, "Filter Cutoff"),
            (71, 32, "Filter Resonance"), 
            (73, 96, "Envelope Attack"),
            (7, 100, "Master Volume"),
            (3, 120, "Clock BPM")
        ]
        
    def log(self, message: str):
        """Log message if verbose mode is enabled"""
        if self.verbose:
            print(f"[MidiTestClient] {message}")
            
    def find_synth_ports(self):
        """Find LVGL Synthesizer MIDI ports"""
        output_ports = self.midi_out.get_ports()  # Where we send TO
        input_ports = self.midi_in.get_ports()    # Where we receive FROM
        
        print(f"🔍 Available output ports (to send TO): {output_ports}")
        print(f"🔍 Available input ports (to receive FROM): {input_ports}")
        
        # Look for LVGL Synth ports
        synth_input = None   # Port to send TO the synthesizer
        synth_output = None  # Port to receive FROM the synthesizer
        
        # Find synthesizer input port (where we send MIDI TO)
        for i, port in enumerate(output_ports):
            if 'RtMidi Input' in port:  # This is the synth's input
                synth_input = i
                print(f"✅ Found synthesizer input port: {port}")
                break
                
        # Find synthesizer output port (where we receive MIDI FROM)  
        for i, port in enumerate(input_ports):
            if 'LVGL Synth Output' in port:  # This is the synth's output
                synth_output = i
                print(f"✅ Found synthesizer output port: {port}")
                break
        
        return synth_input, synth_output
        
    def connect(self) -> bool:
        """Connect to MIDI ports"""
        output_port, input_port = self.find_synth_ports()
        
        if output_port is None:
            self.log(f"❌ Could not find output port '{self.output_port_name}'")
            self.log("Make sure the LVGL synthesizer is running!")
            return False
            
        try:
            # Connect output (where we send commands)
            self.midi_out = rtmidi.MidiOut()
            self.midi_out.open_port(output_port)
            self.log(f"✅ Connected to output port {output_port}")
            
            # Connect input (where we receive feedback) - optional
            if input_port is not None:
                self.midi_in = rtmidi.MidiIn()
                self.midi_in.set_callback(self._midi_callback)
                self.midi_in.open_port(input_port)
                self.log(f"✅ Connected to input port {input_port}")
            else:
                self.log("⚠️ Input port not found - feedback monitoring disabled")
                
            return True
            
        except Exception as e:
            self.log(f"❌ Connection failed: {e}")
            return False
            
    def _midi_callback(self, event, data=None):
        """Handle incoming MIDI messages"""
        message, deltatime = event
        self.messages_received += 1
        
        if len(message) >= 3 and (message[0] & 0xF0) == 0xB0:
            # Control Change message
            channel = (message[0] & 0x0F) + 1
            cc_num = message[1]
            value = message[2]
            self.log(f"📥 Received CC: Ch{channel} CC{cc_num} = {value}")
            
    def send_cc(self, channel: int, controller: int, value: int, name: str = ""):
        """Send MIDI Control Change message"""
        if not self.midi_out:
            print("❌ MIDI output not connected")
            return False
        
        try:
            cc_msg = [0xB0 + (channel - 1), controller, value]
            self.midi_out.send_message(cc_msg)
            desc = f" ({name})" if name else ""
            print(f"📤 Sent CC: Ch{channel} CC{controller} = {value}{desc}")
            return True
        except Exception as e:
            print(f"❌ Failed to send CC: {e}")
            return False
    
    def send_note(self, channel: int, note: int, velocity: int, duration: float = 0.5):
        """Send MIDI Note On/Off with specified duration"""
        if not self.midi_out:
            return False
        
        try:
            # Note On
            note_on = [0x90 + (channel - 1), note, velocity]
            self.midi_out.send_message(note_on)
            print(f"📤 Note On: Ch{channel} Note{note} Vel{velocity}")
            
            # Wait for duration
            time.sleep(duration)
            
            # Note Off
            note_off = [0x80 + (channel - 1), note, 0]
            self.midi_out.send_message(note_off)
            print(f"📤 Note Off: Ch{channel} Note{note}")
            return True
        except Exception as e:
            print(f"❌ Failed to send note: {e}")
            return False
            
    def send_note(self, channel: int, note: int, velocity: int, duration: float = 0.5):
        """Send a Note On/Off sequence"""
        if not self.midi_out:
            return False
            
        try:
            # Note On
            note_on = [0x90 + (channel - 1), note, velocity]
            self.midi_out.send_message(note_on)
            self.log(f"🎵 Note ON: Ch{channel} Note{note} Vel{velocity}")
            
            # Schedule Note Off
            def send_note_off():
                time.sleep(duration)
                note_off = [0x80 + (channel - 1), note, 0]
                self.midi_out.send_message(note_off)
                self.log(f"🎵 Note OFF: Ch{channel} Note{note}")
                
            threading.Thread(target=send_note_off, daemon=True).start()
            self.messages_sent += 2
            return True
            
        except Exception as e:
            self.log(f"❌ Failed to send note: {e}")
            return False
            
    def run_parameter_sweep_test(self):
        """Test all synthesizer parameters with smooth sweeps"""
        self.log("🧪 Starting Parameter Sweep Test...")
        
        for cc_num, initial_value, param_name in self.cc_test_sequence:
            self.log(f"🎛️ Testing {param_name} (CC{cc_num})...")
            
            # Sweep from 0 to 127 and back
            for value in range(0, 128, 8):
                self.send_cc(1, cc_num, value, param_name)
                time.sleep(0.05)  # 50ms between changes
                
            for value in range(127, -1, -8):
                self.send_cc(1, cc_num, value, param_name)
                time.sleep(0.05)
                
            # Reset to reasonable value
            self.send_cc(1, cc_num, initial_value, param_name)
            time.sleep(0.2)
            
    def run_stress_test(self, duration: int = 10):
        """Send rapid MIDI messages to stress-test the system"""
        self.log(f"🔥 Starting {duration}s Stress Test...")
        
        start_time = time.time()
        test_cc = 74  # Filter cutoff
        
        while time.time() - start_time < duration:
            # Rapid parameter changes
            for value in [0, 32, 64, 96, 127]:
                self.send_cc(1, test_cc, value)
                time.sleep(0.01)  # 10ms - very fast!
                
        self.log(f"✅ Stress test completed!")
        
    def run_musical_test(self):
        """Send musical note sequences"""
        self.log("🎶 Starting Musical Test...")
        
        # Play a simple scale
        scale_notes = [60, 62, 64, 65, 67, 69, 71, 72]  # C major scale
        
        for note in scale_notes:
            self.send_note(1, note, 80, 0.3)
            time.sleep(0.4)
            
        # Play some chords
        chords = [
            [60, 64, 67],  # C major
            [65, 69, 72],  # F major  
            [67, 71, 74],  # G major
            [60, 64, 67],  # C major
        ]
        
        for chord in chords:
            for note in chord:
                self.send_note(1, note, 70, 1.0)
            time.sleep(1.2)
            
    def run_comprehensive_test(self):
        """Run all test scenarios"""
        self.log("🚀 Starting Comprehensive MIDI Test Suite...")
        
        if not self.connect():
            return False
            
        try:
            # Wait for synth to be ready
            time.sleep(1.0)
            
            # 1. Parameter sweep test
            self.run_parameter_sweep_test()
            time.sleep(1.0)
            
            # 2. Musical test
            self.run_musical_test()
            time.sleep(1.0)
            
            # 3. Stress test (shorter for demo)
            self.run_stress_test(5)
            
            # Final stats
            self.log(f"📊 Test Results:")
            self.log(f"   Messages Sent: {self.messages_sent}")
            self.log(f"   Messages Received: {self.messages_received}")
            self.log(f"✅ All tests completed successfully!")
            
            return True
            
        except KeyboardInterrupt:
            self.log("⏹️ Test interrupted by user")
            return False
        except Exception as e:
            self.log(f"❌ Test failed: {e}")
            return False
        finally:
            self.disconnect()
    
    def _clock_thread_func(self):
        """MIDI clock thread function"""
        interval = 60.0 / (self.clock_bpm * self.clock_ppqn)  # Time between clock pulses
        
        while self.clock_running:
            if self.midi_out:
                self.midi_out.send_message([self.MIDI_CLOCK])
            time.sleep(interval)
    
    def start_clock(self, bpm: int):
        """Start MIDI clock at specified BPM"""
        if self.clock_running:
            self.stop_clock()
        
        self.clock_bpm = bpm
        self.clock_running = True
        
        # Send MIDI Start message
        if self.midi_out:
            self.midi_out.send_message([self.MIDI_START])
        
        # Start clock thread
        self.clock_thread = threading.Thread(target=self._clock_thread_func, daemon=True)
        self.clock_thread.start()
        
        print(f"🕰️ MIDI Clock started at {bpm} BPM ({self.clock_ppqn} PPQN)")
    
    def stop_clock(self):
        """Stop MIDI clock"""
        if not self.clock_running:
            return
        
        self.clock_running = False
        
        # Send MIDI Stop message
        if self.midi_out:
            self.midi_out.send_message([self.MIDI_STOP])
        
        # Wait for thread to finish
        if self.clock_thread:
            self.clock_thread.join(timeout=1.0)
        
        print("🛑 MIDI Clock stopped")
            
    def disconnect(self):
        """Clean up MIDI connections"""
        if self.midi_out:
            self.midi_out.close_port()
            self.midi_out = None
            
        if self.midi_in:
            self.midi_in.close_port() 
            self.midi_in = None
            
        self.log("🔌 Disconnected from MIDI ports")


def main():
    if len(sys.argv) < 2:
        print("🎹 LVGL Synthesizer MIDI Test Client")
        print("=" * 40)
        print("Usage:")
        print("  clock on <bpm>        - Start MIDI clock at specified BPM")
        print("  clock stop            - Stop MIDI clock")
        print("  cc <controller> <value> - Send CC message")
        print("  note <note> <velocity> - Send note on/off")
        print("  test <type>           - Run automated tests (sweep|stress|musical|all)")
        print("")
        print("Examples:")
        print("  python3 midi_test_client.py clock on 120")
        print("  python3 midi_test_client.py clock stop")
        print("  python3 midi_test_client.py cc 74 64")
        print("  python3 midi_test_client.py test all")
        return 1
    
    command = sys.argv[1].lower()
    
    client = MidiTestClient(verbose=True)
    
    try:
        if not client.connect():
            return 1
        
        if command == "clock":
            if len(sys.argv) < 3:
                print("❌ Clock command requires 'on <bpm>' or 'stop'")
                return 1
            
            action = sys.argv[2].lower()
            if action == "on":
                if len(sys.argv) < 4:
                    print("❌ Clock on requires BPM: clock on <bpm>")
                    return 1
                try:
                    bpm = int(sys.argv[3])
                    if bpm < 30 or bpm > 300:
                        print("❌ BPM must be between 30 and 300")
                        return 1
                    client.start_clock(bpm)
                    print("Press Ctrl+C to stop the clock...")
                    # Keep running until interrupted
                    while True:
                        time.sleep(1)
                except ValueError:
                    print("❌ Invalid BPM value")
                    return 1
                except KeyboardInterrupt:
                    print("\n🛑 Stopping clock...")
                    client.stop_clock()
                    
            elif action == "stop":
                client.stop_clock()
            else:
                print("❌ Clock action must be 'on' or 'stop'")
                return 1
                
        elif command == "cc":
            if len(sys.argv) < 4:
                print("❌ CC command requires: cc <controller> <value>")
                return 1
            try:
                controller = int(sys.argv[2])
                value = int(sys.argv[3])
                if controller < 0 or controller > 127:
                    print("❌ Controller must be 0-127")
                    return 1
                if value < 0 or value > 127:
                    print("❌ Value must be 0-127")
                    return 1
                client.send_cc(1, controller, value)
            except ValueError:
                print("❌ Invalid controller or value")
                return 1
                
        elif command == "note":
            if len(sys.argv) < 4:
                print("❌ Note command requires: note <note> <velocity>")
                return 1
            try:
                note = int(sys.argv[2])
                velocity = int(sys.argv[3])
                if note < 0 or note > 127:
                    print("❌ Note must be 0-127")
                    return 1
                if velocity < 0 or velocity > 127:
                    print("❌ Velocity must be 0-127")
                    return 1
                client.send_note(1, note, velocity, 1.0)
            except ValueError:
                print("❌ Invalid note or velocity")
                return 1
                
        elif command == "test":
            test_type = sys.argv[2].lower() if len(sys.argv) > 2 else "all"
            
            if test_type == "all":
                success = client.run_comprehensive_test()
            elif test_type == "sweep":
                client.run_parameter_sweep_test()
                success = True
            elif test_type == "stress":
                client.run_stress_test(10)
                success = True
            elif test_type == "musical":
                client.run_musical_test()
                success = True
            else:
                print("❌ Test type must be: sweep, stress, musical, or all")
                return 1
                
            if not success:
                return 1
                
        else:
            print(f"❌ Unknown command: {command}")
            return 1
            
    except Exception as e:
        print(f"❌ Error: {e}")
        return 1
    finally:
        client.disconnect()
    
    return 0


if __name__ == "__main__":
    exit(main())
