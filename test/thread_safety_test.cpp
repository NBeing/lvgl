#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include "components/midi/ThreadSafeSubject.h"

class ThreadSafetyTest {
private:
    std::atomic<int> events_received_{0};
    std::atomic<int> events_sent_{0};
    std::atomic<bool> test_running_{false};
    
public:
    class TestObserver : public TypedObserver<MidiEvent> {
    private:
        ThreadSafetyTest* test_;
        
    public:
        TestObserver(ThreadSafetyTest* test) : test_(test) {}
        
        void onEvent(const MidiEvent& event) override {
            test_->events_received_.fetch_add(1);
            // Simulate some processing time
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    };
    
    void runConcurrencyTest() {
        std::cout << "Testing thread safety with concurrent access..." << std::endl;
        
        ThreadSafeSubject<MidiEvent> subject;
        
        // Create multiple observers
        std::vector<std::unique_ptr<TestObserver>> observers;
        for (int i = 0; i < 5; ++i) {
            auto observer = std::make_unique<TestObserver>(this);
            subject.addObserver(observer.get());
            observers.push_back(std::move(observer));
        }
        
        test_running_ = true;
        events_received_ = 0;
        events_sent_ = 0;
        
        // Start multiple producer threads
        std::vector<std::thread> producers;
        for (int i = 0; i < 3; ++i) {
            producers.emplace_back([this, &subject, i]() {
                producerThread(subject, i);
            });
        }
        
        // Start consumer thread (simulates UI thread)
        std::thread consumer([this, &subject]() {
            consumerThread(subject);
        });
        
        // Run test for 5 seconds
        std::this_thread::sleep_for(std::chrono::seconds(5));
        test_running_ = false;
        
        // Wait for all threads to finish
        for (auto& producer : producers) {
            producer.join();
        }
        consumer.join();
        
        // Analyze results
        analyzeThreadSafetyResults();
    }
    
private:
    void producerThread(ThreadSafeSubject<MidiEvent>& subject, int thread_id) {
        while (test_running_) {
            MidiEvent event{0x90, 60, 100, std::chrono::high_resolution_clock::now()};
            subject.enqueueEvent(event);
            events_sent_.fetch_add(1);
            
            // Random delay to simulate real MIDI timing
            std::this_thread::sleep_for(
                std::chrono::microseconds(100 + (thread_id * 50)));
        }
    }
    
    void consumerThread(ThreadSafeSubject<MidiEvent>& subject) {
        while (test_running_) {
            subject.processQueuedEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60Hz
        }
        
        // Process remaining events
        subject.processQueuedEvents();
    }
    
    void analyzeThreadSafetyResults() {
        std::cout << "\n=== Thread Safety Test Results ===" << std::endl;
        std::cout << "Events sent: " << events_sent_.load() << std::endl;
        std::cout << "Events received: " << events_received_.load() << std::endl;
        
        // Calculate event processing rate
        int total_expected = events_sent_.load() * 5; // 5 observers
        int actual_received = events_received_.load();
        
        double success_rate = (double)actual_received / total_expected * 100.0;
        
        std::cout << "Expected events (sent × observers): " << total_expected << std::endl;
        std::cout << "Success rate: " << success_rate << "%" << std::endl;
        
        if (success_rate > 95.0) {
            std::cout << "✅ PASS: Thread safety test successful" << std::endl;
        } else {
            std::cout << "❌ FAIL: Possible race conditions or lost events" << std::endl;
        }
    }
};
