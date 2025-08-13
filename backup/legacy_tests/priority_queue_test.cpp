#include <queue>
#include <chrono>
#include <iostream>
#include "components/midi/PriorityEventQueue.h"

class PriorityQueueTest {
private:
    struct TimestampedEvent {
        MidiEvent event;
        std::chrono::high_resolution_clock::time_point timestamp;
        MidiEventPriority priority;
    };
    
    std::vector<TimestampedEvent> sent_events_;
    std::vector<TimestampedEvent> received_events_;
    
public:
    void runPriorityTest() {
        std::cout << "Testing priority queue ordering..." << std::endl;
        
        PriorityEventQueue<MidiEvent> priority_queue;
        
        // Send events in mixed priority order
        sendMixedPriorityEvents(priority_queue);
        
        // Receive events and check ordering
        receivePriorityEvents(priority_queue);
        
        // Analyze priority ordering
        analyzePriorityResults();
    }
    
private:
    void sendMixedPriorityEvents(PriorityEventQueue<MidiEvent>& queue) {
        sent_events_.clear();
        
        // Send events in deliberately mixed order
        auto now = std::chrono::high_resolution_clock::now();
        
        // Low priority first
        MidiEvent low_event{0xF0, 0x7E, 0x00}; // SysEx
        queue.enqueue(low_event, MidiEventPriority::LOW);
        sent_events_.push_back({low_event, now, MidiEventPriority::LOW});
        
        // Critical priority
        MidiEvent critical_event{0xF8, 0x00, 0x00}; // MIDI Clock
        queue.enqueue(critical_event, MidiEventPriority::CRITICAL);
        sent_events_.push_back({critical_event, now, MidiEventPriority::CRITICAL});
        
        // Medium priority
        MidiEvent medium_event{0xB0, 0x07, 0x7F}; // CC Volume
        queue.enqueue(medium_event, MidiEventPriority::MEDIUM);
        sent_events_.push_back({medium_event, now, MidiEventPriority::MEDIUM});
        
        // High priority
        MidiEvent high_event{0x90, 0x60, 0x7F}; // Note On
        queue.enqueue(high_event, MidiEventPriority::HIGH);
        sent_events_.push_back({high_event, now, MidiEventPriority::HIGH});
        
        // Another critical
        MidiEvent critical2_event{0xFA, 0x00, 0x00}; // Start
        queue.enqueue(critical2_event, MidiEventPriority::CRITICAL);
        sent_events_.push_back({critical2_event, now, MidiEventPriority::CRITICAL});
        
        std::cout << "Sent 5 events in order: LOW, CRITICAL, MEDIUM, HIGH, CRITICAL" << std::endl;
    }
    
    void receivePriorityEvents(PriorityEventQueue<MidiEvent>& queue) {
        received_events_.clear();
        
        MidiEvent event;
        auto receive_time = std::chrono::high_resolution_clock::now();
        
        while (queue.dequeue(event)) {
            // Find the original priority for this event
            MidiEventPriority priority = MidiEventPriority::LOW;
            for (const auto& sent : sent_events_) {
                if (sent.event.status == event.status && 
                    sent.event.data1 == event.data1 &&
                    sent.event.data2 == event.data2) {
                    priority = sent.priority;
                    break;
                }
            }
            
            received_events_.push_back({event, receive_time, priority});
        }
    }
    
    void analyzePriorityResults() {
        std::cout << "\n=== Priority Queue Test Results ===" << std::endl;
        
        if (received_events_.size() != sent_events_.size()) {
            std::cout << "❌ FAIL: Lost events - sent " << sent_events_.size() 
                      << ", received " << received_events_.size() << std::endl;
            return;
        }
        
        std::cout << "Received events in order:" << std::endl;
        bool correct_ordering = true;
        
        for (size_t i = 0; i < received_events_.size(); ++i) {
            const char* priority_name = getPriorityName(received_events_[i].priority);
            std::cout << i + 1 << ". " << priority_name 
                      << " (0x" << std::hex << (int)received_events_[i].event.status << std::dec << ")" 
                      << std::endl;
            
            // Check if priority is correct (higher priority should come first)
            if (i > 0) {
                if (received_events_[i].priority < received_events_[i-1].priority) {
                    std::cout << "❌ Priority violation detected!" << std::endl;
                    correct_ordering = false;
                }
            }
        }
        
        // Expected order: CRITICAL, CRITICAL, HIGH, MEDIUM, LOW
        std::vector<MidiEventPriority> expected_order = {
            MidiEventPriority::CRITICAL,
            MidiEventPriority::CRITICAL,
            MidiEventPriority::HIGH,
            MidiEventPriority::MEDIUM,
            MidiEventPriority::LOW
        };
        
        bool matches_expected = true;
        for (size_t i = 0; i < received_events_.size() && i < expected_order.size(); ++i) {
            if (received_events_[i].priority != expected_order[i]) {
                matches_expected = false;
                break;
            }
        }
        
        if (correct_ordering && matches_expected) {
            std::cout << "✅ PASS: Priority queue ordering correct" << std::endl;
        } else {
            std::cout << "❌ FAIL: Priority queue ordering incorrect" << std::endl;
        }
    }
    
    const char* getPriorityName(MidiEventPriority priority) {
        switch (priority) {
            case MidiEventPriority::CRITICAL: return "CRITICAL";
            case MidiEventPriority::HIGH: return "HIGH";
            case MidiEventPriority::MEDIUM: return "MEDIUM";
            case MidiEventPriority::LOW: return "LOW";
            default: return "UNKNOWN";
        }
    }
};
