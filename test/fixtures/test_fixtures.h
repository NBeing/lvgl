/**
 * @brief Test Fixtures and Mocks for Unified Test Framework
 * 
 * Common mocks and test fixtures used across multiple test categories
 */

#pragma once

#include <functional>
#include <vector>
#include <string>
#include <memory>

namespace TestFixtures {

/**
 * @brief Mock MIDI Interface for testing
 */
class MockMidiInterface {
public:
    struct MidiMessage {
        uint8_t status;
        uint8_t data1;
        uint8_t data2;
        uint64_t timestamp;
    };

    std::vector<MidiMessage> sent_messages;
    std::vector<MidiMessage> received_messages;
    
    void sendMidiMessage(uint8_t status, uint8_t data1, uint8_t data2) {
        sent_messages.push_back({status, data1, data2, getCurrentTime()});
    }
    
    void simulateReceivedMessage(uint8_t status, uint8_t data1, uint8_t data2) {
        received_messages.push_back({status, data1, data2, getCurrentTime()});
        if (message_callback_) {
            message_callback_(status, data1, data2);
        }
    }
    
    void setMessageCallback(std::function<void(uint8_t, uint8_t, uint8_t)> callback) {
        message_callback_ = callback;
    }
    
    void clearMessages() {
        sent_messages.clear();
        received_messages.clear();
    }
    
    size_t getSentMessageCount() const { return sent_messages.size(); }
    size_t getReceivedMessageCount() const { return received_messages.size(); }

private:
    std::function<void(uint8_t, uint8_t, uint8_t)> message_callback_;
    
    uint64_t getCurrentTime() {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
};

/**
 * @brief Mock Parameter Manager for testing
 */
class MockParameterManager {
public:
    struct ParameterChange {
        uint32_t param_id;
        float value;
        uint64_t timestamp;
    };
    
    std::vector<ParameterChange> parameter_changes;
    std::function<void(uint32_t, float)> change_callback_;
    
    void setParameter(uint32_t param_id, float value) {
        parameter_changes.push_back({param_id, value, getCurrentTime()});
        if (change_callback_) {
            change_callback_(param_id, value);
        }
    }
    
    float getParameter(uint32_t param_id) const {
        auto it = std::find_if(parameter_changes.rbegin(), parameter_changes.rend(),
            [param_id](const ParameterChange& change) {
                return change.param_id == param_id;
            });
        return it != parameter_changes.rend() ? it->value : 0.0f;
    }
    
    void setChangeCallback(std::function<void(uint32_t, float)> callback) {
        change_callback_ = callback;
    }
    
    void clearChanges() {
        parameter_changes.clear();
    }
    
    size_t getChangeCount() const { return parameter_changes.size(); }

private:
    uint64_t getCurrentTime() {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
};

/**
 * @brief Mock Clock Manager for testing
 */
class MockMidiClockManager {
public:
    std::function<void(int)> clock_tick_callback_;
    std::function<void(int, int)> transport_changed_callback_;
    std::function<void(float)> bpm_changed_callback_;
    
    int current_tick_ = 0;
    float current_bpm_ = 120.0f;
    bool is_playing_ = false;
    
    void setClockTickCallback(std::function<void(int)> callback) {
        clock_tick_callback_ = callback;
    }
    
    void setTransportChangedCallback(std::function<void(int, int)> callback) {
        transport_changed_callback_ = callback;
    }
    
    void setBpmChangedCallback(std::function<void(float)> callback) {
        bpm_changed_callback_ = callback;
    }
    
    void simulateClockTick() {
        current_tick_++;
        if (clock_tick_callback_) {
            clock_tick_callback_(current_tick_);
        }
    }
    
    void simulateBpmChange(float new_bpm) {
        current_bpm_ = new_bpm;
        if (bpm_changed_callback_) {
            bpm_changed_callback_(new_bpm);
        }
    }
    
    void simulateTransportChange(int old_state, int new_state) {
        is_playing_ = (new_state == 1);
        if (transport_changed_callback_) {
            transport_changed_callback_(old_state, new_state);
        }
    }
    
    int getCurrentTick() const { return current_tick_; }
    float getCurrentBpm() const { return current_bpm_; }
    bool isPlaying() const { return is_playing_; }
    
    void reset() {
        current_tick_ = 0;
        current_bpm_ = 120.0f;
        is_playing_ = false;
    }
};

/**
 * @brief Test data generators
 */
class TestData {
public:
    static std::vector<uint8_t> generateMidiSequence(size_t length) {
        std::vector<uint8_t> sequence;
        for (size_t i = 0; i < length; ++i) {
            sequence.push_back(0x90);  // Note on
            sequence.push_back(60 + (i % 24));  // Note number
            sequence.push_back(100);   // Velocity
        }
        return sequence;
    }
    
    static std::vector<float> generateParameterSequence(size_t length, float min_val = 0.0f, float max_val = 1.0f) {
        std::vector<float> sequence;
        for (size_t i = 0; i < length; ++i) {
            float normalized = (float)i / (length - 1);
            sequence.push_back(min_val + normalized * (max_val - min_val));
        }
        return sequence;
    }
};

} // namespace TestFixtures
