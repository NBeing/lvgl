#pragma once

#ifndef ESP32_BUILD

#include "MidiInterface.h"
#include <vector>
#include <memory>
#include <thread>
#include <atomic>

// Forward declarations for RtMidi
class RtMidiOut;
class RtMidiIn;

class DesktopMidiInterface : public MidiInterface {
private:
    std::unique_ptr<RtMidiOut> midi_out_;
    std::unique_ptr<RtMidiIn> midi_in_;
    std::atomic<bool> initialized_;
    std::atomic<bool> connected_;
    MidiReceiveCallback receive_callback_;
    std::thread midi_thread_;
    std::atomic<bool> running_;
    
    std::vector<std::string> available_outputs_;
    std::vector<std::string> available_inputs_;
    int selected_output_port_;
    int selected_input_port_;
    
public:
    DesktopMidiInterface();
    ~DesktopMidiInterface() override;
    
    bool initialize() override;
    void sendControlChange(uint8_t channel, uint8_t cc_number, uint8_t value) override;
    void sendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) override;
    void sendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) override;
    void sendPitchBend(uint8_t channel, uint16_t value) override;
    
    void setReceiveCallback(MidiReceiveCallback callback) override;
    bool isConnected() const override;
    const char* getConnectionStatus() const override;
    void update() override;
    
    // Desktop-specific methods
    std::vector<std::string> getAvailableOutputPorts() const;
    std::vector<std::string> getAvailableInputPorts() const;
    bool selectOutputPort(int port_index);
    bool selectInputPort(int port_index);
    
private:
    void scanMidiPorts();
    void sendMidiMessage(const std::vector<uint8_t>& message);
    void midiReceiveThread();
    static void midiInputCallback(double timestamp, std::vector<uint8_t>* message, void* user_data);
};

#endif // !ESP32_BUILD