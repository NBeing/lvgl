#include <atomic>
#include <thread>
#include <chrono>
#include <random>
#include "components/midi/RealTimeMidiProcessor.h"

class MidiLoadTest {
private:
    std::atomic<int> messages_processed_{0};
    std::atomic<int> messages_dropped_{0};
    std::atomic<bool> test_running_{false};
    
public:
    void runLoadTest() {
        std::cout << "Running MIDI load test..." << std::endl;
        
        RealTimeMidiProcessor processor;
        
        // Set up test observer to count processed messages
        TestLoadObserver observer(this);
        processor.getNoteSubject().addObserver(&observer);
        processor.getCCSubject().addObserver(&observer);
        processor.getClockSubject().addObserver(&observer);
        
        // Start processor
        processor.startRealTimeProcessing();
        
        test_running_ = true;
        messages_processed_ = 0;
        messages_dropped_ = 0;
        
        // Generate high-frequency MIDI traffic
        std::thread load_generator([this, &processor]() {
            generateHighFrequencyMidi(processor);
        });
        
        // Monitor performance for 10 seconds
        auto start_time = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(10)) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            int processed = messages_processed_.load();
            int dropped = messages_dropped_.load();
            
            std::cout << "Processed: " << processed 
                      << ", Dropped: " << dropped 
                      << ", Rate: " << processed << " msg/sec" << std::endl;
        }
        
        test_running_ = false;
        load_generator.join();
        
        processor.stopRealTimeProcessing();
        
        analyzeLoadResults();
    }
    
private:
    class TestLoadObserver : public TypedObserver<MidiEvent> {
    private:
        MidiLoadTest* test_;
    public:
        TestLoadObserver(MidiLoadTest* test) : test_(test) {}
        void onEvent(const MidiEvent& event) override {
            test_->messages_processed_.fetch_add(1);
        }
    };
    
    void generateHighFrequencyMidi(RealTimeMidiProcessor& processor) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> note_dist(36, 96);  // Piano range
        std::uniform_int_distribution<> vel_dist(1, 127);   // Velocity range
        std::uniform_int_distribution<> cc_dist(0, 127);    // CC range
        
        int messages_sent = 0;
        
        while (test_running_) {
            // Generate different types of MIDI messages
            
            // Note events (50% of traffic)
            if (messages_sent % 2 == 0) {
                MidiMessage note_on = MidiMessage::noteOn(note_dist(gen), vel_dist(gen));
                if (!processor.enqueueMidiMessage(note_on)) {
                    messages_dropped_.fetch_add(1);
                }
                messages_sent++;
            }
            
            // CC events (30% of traffic)
            if (messages_sent % 10 < 3) {
                MidiMessage cc = MidiMessage::controlChange(1, cc_dist(gen) % 128, cc_dist(gen));
                if (!processor.enqueueMidiMessage(cc)) {
                    messages_dropped_.fetch_add(1);
                }
                messages_sent++;
            }
            
            // Clock events (20% of traffic) - high frequency
            if (messages_sent % 5 == 0) {
                MidiMessage clock = MidiMessage::clock();
                if (!processor.enqueueMidiMessage(clock)) {
                    messages_dropped_.fetch_add(1);
                }
                messages_sent++;
            }
            
            // Send at very high rate - 10kHz+
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
    
    void analyzeLoadResults() {
        int total_processed = messages_processed_.load();
        int total_dropped = messages_dropped_.load();
        int total_sent = total_processed + total_dropped;
        
        std::cout << "\n=== MIDI Load Test Results ===" << std::endl;
        std::cout << "Total messages sent: " << total_sent << std::endl;
        std::cout << "Messages processed: " << total_processed << std::endl;
        std::cout << "Messages dropped: " << total_dropped << std::endl;
        
        if (total_sent > 0) {
            double success_rate = (double)total_processed / total_sent * 100.0;
            double throughput = total_processed / 10.0; // 10 second test
            
            std::cout << "Success rate: " << success_rate << "%" << std::endl;
            std::cout << "Throughput: " << throughput << " messages/second" << std::endl;
            
            // Performance criteria
            bool passed = true;
            
            if (success_rate < 99.0) {
                std::cout << "❌ FAIL: Success rate < 99%" << std::endl;
                passed = false;
            }
            
            if (throughput < 1000.0) { // Should handle at least 1000 msg/sec
                std::cout << "❌ FAIL: Throughput < 1000 msg/sec" << std::endl;
                passed = false;
            }
            
            if (passed) {
                std::cout << "✅ PASS: Load test successful" << std::endl;
            }
        } else {
            std::cout << "❌ FAIL: No messages processed" << std::endl;
        }
    }
};
