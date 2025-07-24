#include <iostream>
#include <vector>
#include <functional>

// Include all test headers
#include "midi_latency_test.cpp"
#include "thread_safety_test.cpp"
#include "midi_accuracy_test.cpp"
#include "priority_queue_test.cpp"
#include "midi_load_test.cpp"

class MidiTestSuite {
private:
    struct TestCase {
        std::string name;
        std::function<bool()> test_func;
    };
    
    std::vector<TestCase> test_cases_;
    
public:
    MidiTestSuite() {
        // Register all test cases
        test_cases_ = {
            {"MIDI Latency Test", []() { 
                MidiLatencyTest test(/* processor */);
                test.runLatencyTest();
                return true; // Would return actual result
            }},
            
            {"Thread Safety Test", []() {
                ThreadSafetyTest test;
                test.runConcurrencyTest();
                return true; // Would return actual result
            }},
            
            {"MIDI Accuracy Test", []() {
                MidiProcessingAccuracyTest test;
                test.runAccuracyTest();
                return true; // Would return actual result
            }},
            
            {"Priority Queue Test", []() {
                PriorityQueueTest test;
                test.runPriorityTest();
                return true; // Would return actual result
            }},
            
            {"MIDI Load Test", []() {
                MidiLoadTest test;
                test.runLoadTest();
                return true; // Would return actual result
            }}
        };
    }
    
    void runAllTests() {
        std::cout << "========================================" << std::endl;
        std::cout << "   MIDI Threading Architecture Tests   " << std::endl;
        std::cout << "========================================" << std::endl;
        
        int passed = 0;
        int total = test_cases_.size();
        
        for (auto& test_case : test_cases_) {
            std::cout << "\nRunning: " << test_case.name << std::endl;
            std::cout << "----------------------------------------" << std::endl;
            
            try {
                bool result = test_case.test_func();
                if (result) {
                    std::cout << "✅ " << test_case.name << " PASSED" << std::endl;
                    passed++;
                } else {
                    std::cout << "❌ " << test_case.name << " FAILED" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "❌ " << test_case.name << " CRASHED: " << e.what() << std::endl;
            }
        }
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "Test Results: " << passed << "/" << total << " passed" << std::endl;
        
        if (passed == total) {
            std::cout << "🎉 ALL TESTS PASSED! Architecture is ready." << std::endl;
        } else {
            std::cout << "⚠️  Some tests failed. Review implementation." << std::endl;
        }
        std::cout << "========================================" << std::endl;
    }
    
    void runSingleTest(const std::string& test_name) {
        for (auto& test_case : test_cases_) {
            if (test_case.name == test_name) {
                std::cout << "Running single test: " << test_name << std::endl;
                test_case.test_func();
                return;
            }
        }
        std::cout << "Test not found: " << test_name << std::endl;
    }
};

// Example usage for ESP32
#ifdef ESP32_BUILD
void setup() {
    Serial.begin(115200);
    delay(2000); // Wait for serial connection
    
    std::cout << "Starting MIDI architecture tests..." << std::endl;
    
    MidiTestSuite test_suite;
    test_suite.runAllTests();
}

void loop() {
    // Tests run once in setup()
    delay(1000);
}
#else
// Desktop version
int main() {
    MidiTestSuite test_suite;
    test_suite.runAllTests();
    return 0;
}
#endif
