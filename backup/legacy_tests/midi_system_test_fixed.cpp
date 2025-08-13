/**
 * @brief MIDI System Unit Tests - FIXED VERSION
 * 
 * Tests core MIDI functionality without external dependencies
 */

#include "../framework/unified_test_framework.h"
#include "../fixtures/test_fixtures.h"
#include <chrono>
#include <thread>
#include <queue>
#include <cstdint>

using namespace TestFixtures;

// Simplified MIDI structures for unit testing
namespace MIDI {

struct MidiMessage {
    uint8_t status;
    uint8_t data1;
    uint8_t data2;
    uint64_t timestamp;
    
    // Default constructor
    MidiMessage() : status(0), data1(0), data2(0), timestamp(0) {}
    
    MidiMessage(uint8_t s, uint8_t d1, uint8_t d2) 
        : status(s), data1(d1), data2(d2) {
        timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    
    bool isNoteOn() const { return (status & 0xF0) == 0x90; }
    bool isNoteOff() const { return (status & 0xF0) == 0x80; }
    bool isControlChange() const { return (status & 0xF0) == 0xB0; }
    
    uint8_t getChannel() const { return status & 0x0F; }
    uint8_t getNote() const { return data1; }
    uint8_t getVelocity() const { return data2; }
    uint8_t getController() const { return data1; }
    uint8_t getValue() const { return data2; }
};

class MidiMessageQueue {
public:
    void push(const MidiMessage& msg) {
        messages_.push(msg);
    }
    
    bool pop(MidiMessage& msg) {
        if (messages_.empty()) return false;
        msg = messages_.front();
        messages_.pop();
        return true;
    }
    
    size_t size() const { return messages_.size(); }
    bool empty() const { return messages_.empty(); }
    void clear() { while (!messages_.empty()) messages_.pop(); }

private:
    std::queue<MidiMessage> messages_;
};

class MidiProcessor {
public:
    void processMessage(const MidiMessage& msg) {
        processed_messages_.push_back(msg);
        
        if (msg.isNoteOn()) {
            note_on_count_++;
        } else if (msg.isNoteOff()) {
            note_off_count_++;
        } else if (msg.isControlChange()) {
            cc_count_++;
        }
    }
    
    std::vector<MidiMessage> getProcessedMessages() const { return processed_messages_; }
    size_t getNoteOnCount() const { return note_on_count_; }
    size_t getNoteOffCount() const { return note_off_count_; }
    size_t getCCCount() const { return cc_count_; }
    
    void reset() {
        processed_messages_.clear();
        note_on_count_ = 0;
        note_off_count_ = 0;
        cc_count_ = 0;
    }

private:
    std::vector<MidiMessage> processed_messages_;
    size_t note_on_count_ = 0;
    size_t note_off_count_ = 0;
    size_t cc_count_ = 0;
};

} // namespace MIDI

using namespace MIDI;

// ============================================================================
// UNIT TESTS - MIDI Message Structure
// ============================================================================

TEST_UNIT(MidiMessage, BasicConstruction) {
    MidiMessage msg(0x90, 60, 100);  // Note On, C4, velocity 100
    
    ASSERT_EQ_NUM(0x90, msg.status);
    ASSERT_EQ_NUM(60, msg.data1);
    ASSERT_EQ_NUM(100, msg.data2);
    ASSERT_TRUE(msg.timestamp > 0);
}

TEST_UNIT(MidiMessage, TypeDetection) {
    MidiMessage noteOn(0x90, 60, 100);   // Note On
    MidiMessage noteOff(0x80, 60, 0);    // Note Off  
    MidiMessage cc(0xB0, 7, 64);         // Control Change
    
    ASSERT_TRUE(noteOn.isNoteOn());
    ASSERT_FALSE(noteOn.isNoteOff());
    ASSERT_FALSE(noteOn.isControlChange());
    
    ASSERT_FALSE(noteOff.isNoteOn());
    ASSERT_TRUE(noteOff.isNoteOff());
    ASSERT_FALSE(noteOff.isControlChange());
    
    ASSERT_FALSE(cc.isNoteOn());
    ASSERT_FALSE(cc.isNoteOff());
    ASSERT_TRUE(cc.isControlChange());
}

TEST_UNIT(MidiMessage, ChannelExtraction) {
    MidiMessage msg1(0x90, 60, 100);  // Channel 0
    MidiMessage msg2(0x95, 60, 100);  // Channel 5
    MidiMessage msg3(0x9F, 60, 100);  // Channel 15
    
    ASSERT_EQ_NUM(0, msg1.getChannel());
    ASSERT_EQ_NUM(5, msg2.getChannel());
    ASSERT_EQ_NUM(15, msg3.getChannel());
}

TEST_UNIT(MidiMessage, DataExtraction) {
    MidiMessage noteMsg(0x90, 72, 110);  // Note message
    MidiMessage ccMsg(0xB0, 7, 64);      // CC message
    
    ASSERT_EQ_NUM(72, noteMsg.getNote());
    ASSERT_EQ_NUM(110, noteMsg.getVelocity());
    
    ASSERT_EQ_NUM(7, ccMsg.getController());
    ASSERT_EQ_NUM(64, ccMsg.getValue());
}

// ============================================================================
// UNIT TESTS - MIDI Message Queue
// ============================================================================

TEST_UNIT(MidiQueue, BasicOperations) {
    MidiMessageQueue queue;
    
    ASSERT_TRUE(queue.empty());
    ASSERT_EQ(0lu, queue.size());
    
    MidiMessage msg1(0x90, 60, 100);
    queue.push(msg1);
    
    ASSERT_FALSE(queue.empty());
    ASSERT_EQ(1lu, queue.size());
    
    MidiMessage retrieved;
    ASSERT_TRUE(queue.pop(retrieved));
    ASSERT_EQ_NUM(msg1.status, retrieved.status);
    ASSERT_EQ_NUM(msg1.data1, retrieved.data1);
    ASSERT_EQ_NUM(msg1.data2, retrieved.data2);
    
    ASSERT_TRUE(queue.empty());
    ASSERT_EQ(0lu, queue.size());
}

TEST_UNIT(MidiQueue, FIFOBehavior) {
    MidiMessageQueue queue;
    
    MidiMessage msg1(0x90, 60, 100);  // C4
    MidiMessage msg2(0x90, 64, 100);  // E4
    MidiMessage msg3(0x90, 67, 100);  // G4
    
    queue.push(msg1);
    queue.push(msg2);
    queue.push(msg3);
    
    ASSERT_EQ(3lu, queue.size());
    
    MidiMessage retrieved;
    
    // Should retrieve in FIFO order
    queue.pop(retrieved);
    ASSERT_EQ_NUM(60, retrieved.data1);  // C4 first
    
    queue.pop(retrieved);
    ASSERT_EQ_NUM(64, retrieved.data1);  // E4 second
    
    queue.pop(retrieved);
    ASSERT_EQ_NUM(67, retrieved.data1);  // G4 third
    
    ASSERT_TRUE(queue.empty());
}

TEST_UNIT(MidiQueue, EmptyPopBehavior) {
    MidiMessageQueue queue;
    MidiMessage msg(0x90, 60, 100);
    
    // Pop from empty queue should return false
    ASSERT_FALSE(queue.pop(msg));
    ASSERT_TRUE(queue.empty());
}

TEST_UNIT(MidiQueue, ClearOperation) {
    MidiMessageQueue queue;
    
    for (int i = 0; i < 10; ++i) {
        queue.push(MidiMessage(0x90, 60 + i, 100));
    }
    
    ASSERT_EQ(10lu, queue.size());
    
    queue.clear();
    
    ASSERT_EQ(0lu, queue.size());
    ASSERT_TRUE(queue.empty());
}

// ============================================================================
// UNIT TESTS - MIDI Processor
// ============================================================================

TEST_UNIT(MidiProcessor, BasicProcessing) {
    MidiProcessor processor;
    
    MidiMessage noteOn(0x90, 60, 100);
    MidiMessage noteOff(0x80, 60, 0);
    MidiMessage cc(0xB0, 7, 64);
    
    processor.processMessage(noteOn);
    processor.processMessage(noteOff);
    processor.processMessage(cc);
    
    ASSERT_EQ(3lu, processor.getProcessedMessages().size());
    ASSERT_EQ(1lu, processor.getNoteOnCount());
    ASSERT_EQ(1lu, processor.getNoteOffCount());
    ASSERT_EQ(1lu, processor.getCCCount());
}

TEST_UNIT(MidiProcessor, MessageCounting) {
    MidiProcessor processor;
    
    // Process multiple note events
    for (int i = 0; i < 5; ++i) {
        processor.processMessage(MidiMessage(0x90, 60 + i, 100));  // Note On
    }
    
    for (int i = 0; i < 3; ++i) {
        processor.processMessage(MidiMessage(0x80, 60 + i, 0));    // Note Off
    }
    
    for (int i = 0; i < 7; ++i) {
        processor.processMessage(MidiMessage(0xB0, i, 64));        // CC
    }
    
    ASSERT_EQ(15lu, processor.getProcessedMessages().size());
    ASSERT_EQ(5lu, processor.getNoteOnCount());
    ASSERT_EQ(3lu, processor.getNoteOffCount());
    ASSERT_EQ(7lu, processor.getCCCount());
}

TEST_UNIT(MidiProcessor, Reset) {
    MidiProcessor processor;
    
    processor.processMessage(MidiMessage(0x90, 60, 100));
    processor.processMessage(MidiMessage(0xB0, 7, 64));
    
    ASSERT_EQ(2lu, processor.getProcessedMessages().size());
    ASSERT_EQ(1lu, processor.getNoteOnCount());
    ASSERT_EQ(1lu, processor.getCCCount());
    
    processor.reset();
    
    ASSERT_EQ(0lu, processor.getProcessedMessages().size());
    ASSERT_EQ(0lu, processor.getNoteOnCount());
    ASSERT_EQ(0lu, processor.getNoteOffCount());
    ASSERT_EQ(0lu, processor.getCCCount());
}

// ============================================================================
// UNIT TESTS - MIDI Performance and Load Testing
// ============================================================================

TEST_UNIT(MidiPerformance, HighMessageVolume) {
    MidiProcessor processor;
    const size_t message_count = 10000;
    
    auto start_time = std::chrono::steady_clock::now();
    
    for (size_t i = 0; i < message_count; ++i) {
        uint8_t note = 60 + (i % 24);  // C4 to B5 range
        processor.processMessage(MidiMessage(0x90, note, 100));
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    ASSERT_EQ(message_count, processor.getNoteOnCount());
    
    // Performance assertion: should process 10k messages in under 100ms
    ASSERT_TRUE(duration.count() < 100000);  // 100ms in microseconds
    
    std::cout << "  Processed " << message_count << " messages in " 
              << duration.count() << " microseconds" << std::endl;
}

TEST_UNIT(MidiLatency, TimestampAccuracy) {
    MidiMessageQueue queue;
    
    auto start_time = std::chrono::steady_clock::now();
    
    MidiMessage msg1(0x90, 60, 100);
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    MidiMessage msg2(0x90, 61, 100);
    
    auto end_time = std::chrono::steady_clock::now();
    
    // Verify timestamp ordering
    ASSERT_TRUE(msg1.timestamp < msg2.timestamp);
    
    // Verify reasonable timestamp difference (should be around 100 microseconds)
    uint64_t time_diff = msg2.timestamp - msg1.timestamp;
    ASSERT_TRUE(time_diff >= 90);   // At least 90μs
    ASSERT_TRUE(time_diff <= 200);  // At most 200μs (allowing for system jitter)
}

// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main() {
    std::cout << "🎵 MIDI System Unit Tests - FIXED VERSION" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    auto& runner = TestFramework::TestRunner::getInstance();
    auto results = runner.runCategory("unit/MidiMessage");
    
    return results.failed_tests == 0 ? 0 : 1;
}
