#include <chrono>
#include <vector>
#include <algorithm>
#include <iostream>
#include "components/midi/RealTimeMidiProcessor.h"

class MidiLatencyTest {
private:
    std::vector<std::chrono::microseconds> latencies_;
    RealTimeMidiProcessor* midi_processor_;
    
public:
    MidiLatencyTest(RealTimeMidiProcessor* processor) 
        : midi_processor_(processor) {}
    
    void runLatencyTest(int num_messages = 1000) {
        std::cout << "Testing MIDI latency with " << num_messages << " messages..." << std::endl;
        
        latencies_.clear();
        latencies_.reserve(num_messages);
        
        for (int i = 0; i < num_messages; ++i) {
            testSingleMessage();
            vTaskDelay(1); // 1ms between tests
        }
        
        analyzeResults();
    }
    
private:
    void testSingleMessage() {
        // Create test MIDI message
        MidiMessage test_msg = MidiMessage::noteOn(60, 100); // Middle C
        
        // Record timestamp when message is sent
        auto send_time = std::chrono::high_resolution_clock::now();
        
        // Send to MIDI processor
        midi_processor_->enqueueMidiMessage(test_msg);
        
        // Wait for processing and measure when it's handled
        // (This requires a test observer that records processing time)
        auto process_time = waitForProcessing(test_msg);
        
        if (process_time != std::chrono::high_resolution_clock::time_point{}) {
            auto latency = std::chrono::duration_cast<std::chrono::microseconds>(
                process_time - send_time);
            latencies_.push_back(latency);
        }
    }
    
    std::chrono::high_resolution_clock::time_point waitForProcessing(const MidiMessage& msg) {
        // Implementation depends on your test observer setup
        // Should return timestamp when message was actually processed
        return std::chrono::high_resolution_clock::now(); // Placeholder
    }
    
    void analyzeResults() {
        if (latencies_.empty()) {
            std::cout << "No latency data collected!" << std::endl;
            return;
        }
        
        std::sort(latencies_.begin(), latencies_.end());
        
        auto min_latency = latencies_.front();
        auto max_latency = latencies_.back();
        auto median_latency = latencies_[latencies_.size() / 2];
        
        auto sum = std::accumulate(latencies_.begin(), latencies_.end(), 
                                  std::chrono::microseconds{0});
        auto avg_latency = sum / latencies_.size();
        
        std::cout << "\n=== MIDI Latency Test Results ===" << std::endl;
        std::cout << "Messages tested: " << latencies_.size() << std::endl;
        std::cout << "Min latency: " << min_latency.count() << " μs" << std::endl;
        std::cout << "Max latency: " << max_latency.count() << " μs" << std::endl;
        std::cout << "Avg latency: " << avg_latency.count() << " μs" << std::endl;
        std::cout << "Median latency: " << median_latency.count() << " μs" << std::endl;
        
        // Performance criteria
        bool passed = true;
        if (avg_latency > std::chrono::microseconds(1000)) { // 1ms
            std::cout << "❌ FAIL: Average latency > 1ms" << std::endl;
            passed = false;
        }
        if (max_latency > std::chrono::microseconds(5000)) { // 5ms
            std::cout << "❌ FAIL: Max latency > 5ms" << std::endl;
            passed = false;
        }
        
        if (passed) {
            std::cout << "✅ PASS: Latency within acceptable limits" << std::endl;
        }
    }
};
