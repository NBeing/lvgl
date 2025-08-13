/**
 * @brief Comprehensive RT-Safe Parameter Manager Tests
 * 
 * Tests the complete parameter management system:
 * - Parameter registration and validation
 * - RT-safe parameter access and modification
 * - Parameter smoothing and interpolation
 * - Thread safety and performance
 * - Event-driven parameter change notifications
 * - Preset management capabilities
 */

#include "TestFramework.h"
#include "components/parameter/RTSafeParameterManager.h"
#include "components/threading/RTSafeEventDistributor.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <iostream>
#include <iomanip>

using namespace RTSafe;

/**
 * @brief Mock parameter change observer for testing callbacks
 */
class ParameterChangeObserver {
private:
    std::mutex changes_mutex_;
    std::vector<std::tuple<uint32_t, float, float>> parameter_changes_; // param_id, old_value, new_value
    
public:
    void onParameterChange(uint32_t parameter_id, float old_value, float new_value) {
        std::lock_guard<std::mutex> lock(changes_mutex_);
        parameter_changes_.emplace_back(parameter_id, old_value, new_value);
        
        std::cout << "🎛️  Parameter " << parameter_id << ": " 
                  << old_value << " → " << new_value << std::endl;
    }
    
    size_t getChangeCount() {
        std::lock_guard<std::mutex> lock(changes_mutex_);
        return parameter_changes_.size();
    }
    
    bool wasParameterChanged(uint32_t parameter_id) {
        std::lock_guard<std::mutex> lock(changes_mutex_);
        for (const auto& change : parameter_changes_) {
            if (std::get<0>(change) == parameter_id) {
                return true;
            }
        }
        return false;
    }
    
    void clearChanges() {
        std::lock_guard<std::mutex> lock(changes_mutex_);
        parameter_changes_.clear();
    }
};

/**
 * @brief RT-Safe Parameter Manager Test Suite
 */
class RTSafeParameterManagerTests : public TestSuite {
private:
    std::unique_ptr<RTSafeEventDistributor> event_distributor_;
    std::unique_ptr<RTSafeParameterManager> parameter_manager_;
    std::unique_ptr<ParameterChangeObserver> observer_;
    
public:
    RTSafeParameterManagerTests() : TestSuite("RT-Safe Parameter Manager Tests") {}
    
    void setUp() override {
        std::cout << "🎛️  Starting RT-Safe Parameter Manager Tests" << std::endl;
        
        // Create event distributor
        event_distributor_ = std::make_unique<RTSafeEventDistributor>();
        ASSERT_TRUE(event_distributor_->initialize());
        
        // Create parameter manager
        parameter_manager_ = std::make_unique<RTSafeParameterManager>(event_distributor_.get());
        ASSERT_TRUE(parameter_manager_->initialize());
        
        // Set sample rate for smoothing
        parameter_manager_->setSampleRate(44100.0f);
        
        // Create observer
        observer_ = std::make_unique<ParameterChangeObserver>();
        parameter_manager_->setParameterChangeCallback(
            [this](uint32_t param_id, float old_val, float new_val) {
                observer_->onParameterChange(param_id, old_val, new_val);
            }
        );
    }
    
    void tearDown() override {
        parameter_manager_->shutdown();
        event_distributor_->shutdown();
        std::cout << "✅ RT-Safe Parameter Manager Tests Completed" << std::endl;
    }
    
    void runTests() override {
        testParameterRegistration();
        testParameterAccess();
        testParameterValidation();
        testParameterSmoothing();
        testThreadSafety();
        testRTPerformance();
        testParameterCategories();
        testEventNotifications();
        testStatisticsAndMonitoring();
    }
    
    // Test 1: Parameter Registration and Metadata
    void testParameterRegistration() {
        TEST_SUITE("Parameter Registration and Metadata");
        
        TEST("Register custom parameter with full metadata") {
            RTSafeParameterManager::ParameterDefinition custom_param;
            custom_param.parameter_id = 9001;
            custom_param.category = RTSafeParameterManager::ParameterCategory::EFFECTS;
            custom_param.type = RTSafeParameterManager::ParameterType::CONTINUOUS;
            custom_param.name = "Reverb Room Size";
            custom_param.short_name = "Room";
            custom_param.units = "%";
            custom_param.min_value = 0.0f;
            custom_param.max_value = 100.0f;
            custom_param.default_value = 50.0f;
            custom_param.logarithmic = false;
            custom_param.smoothing_time_ms = 100;
            
            bool success = parameter_manager_->registerParameter(custom_param);
            ASSERT_TRUE(success);
            
            // Verify parameter exists
            ASSERT_TRUE(parameter_manager_->hasParameter(9001));
            
            // Check parameter definition
            const auto* definition = parameter_manager_->getParameterDefinition(9001);
            ASSERT_NE(nullptr, definition);
            ASSERT_EQ(9001, definition->parameter_id);
            ASSERT_EQ("Reverb Room Size", definition->name);
            ASSERT_EQ(50.0f, parameter_manager_->getParameter(9001)); // Should be default value
            
        } END_TEST();
        
        TEST("Default synthesizer parameters are registered") {
            // Check that default parameters exist
            ASSERT_TRUE(parameter_manager_->hasParameter(1001)); // Osc1 Frequency
            ASSERT_TRUE(parameter_manager_->hasParameter(1002)); // Filter Cutoff
            ASSERT_TRUE(parameter_manager_->hasParameter(1003)); // Filter Resonance
            ASSERT_TRUE(parameter_manager_->hasParameter(4001)); // Master Volume
            ASSERT_TRUE(parameter_manager_->hasParameter(2001)); // Envelope Attack
            ASSERT_TRUE(parameter_manager_->hasParameter(2002)); // Envelope Decay
            
            // Check default values
            ASSERT_EQ(440.0f, parameter_manager_->getParameter(1001));  // Osc1 default
            ASSERT_EQ(1000.0f, parameter_manager_->getParameter(1002)); // Filter default
            ASSERT_EQ(0.8f, parameter_manager_->getParameter(4001));    // Volume default
            
        } END_TEST();
        
        TEST("Invalid parameter registration rejected") {
            RTSafeParameterManager::ParameterDefinition invalid_param;
            invalid_param.parameter_id = 0; // Invalid ID
            
            bool success = parameter_manager_->registerParameter(invalid_param);
            ASSERT_FALSE(success);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 2: Parameter Access and Modification
    void testParameterAccess() {
        TEST_SUITE("Parameter Access and Modification");
        
        TEST("Set and get parameter values") {
            parameter_manager_->resetStatistics();
            observer_->clearChanges();
            
            // Set filter cutoff to 2000 Hz
            bool success = parameter_manager_->setParameter(1002, 2000.0f, true);
            ASSERT_TRUE(success);
            
            // Verify value was set
            float value = parameter_manager_->getParameter(1002);
            ASSERT_TRUE(std::abs(value - 2000.0f) < 0.1f);
            
            // Verify callback was triggered
            ASSERT_TRUE(observer_->wasParameterChanged(1002));
            
        } END_TEST();
        
        TEST("Target vs current values during smoothing") {
            // Set a value with smoothing (not immediate)
            parameter_manager_->setParameter(1002, 5000.0f, false);
            
            // Target should be set immediately
            float target = parameter_manager_->getParameterTarget(1002);
            ASSERT_TRUE(std::abs(target - 5000.0f) < 0.1f);
            
            // Current value should still be old value initially
            float current = parameter_manager_->getParameter(1002);
            ASSERT_TRUE(current < 5000.0f);
            
        } END_TEST();
        
        TEST("Nonexistent parameter returns zero") {
            float value = parameter_manager_->getParameter(99999);
            ASSERT_EQ(0.0f, value);
            
            bool success = parameter_manager_->setParameter(99999, 1.0f);
            ASSERT_FALSE(success);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 3: Parameter Validation and Range Checking
    void testParameterValidation() {
        TEST_SUITE("Parameter Validation and Range Checking");
        
        TEST("Values clamped to valid range") {
            // Try to set filter cutoff above maximum (20kHz)
            parameter_manager_->setParameter(1002, 25000.0f, true);
            float value = parameter_manager_->getParameter(1002);
            ASSERT_TRUE(value <= 20000.0f); // Should be clamped
            
            // Try to set below minimum (20 Hz)
            parameter_manager_->setParameter(1002, 10.0f, true);
            value = parameter_manager_->getParameter(1002);
            ASSERT_TRUE(value >= 20.0f); // Should be clamped
            
        } END_TEST();
        
        TEST("Step size quantization works") {
            // Filter resonance has step size of 0.01
            parameter_manager_->setParameter(1003, 0.237f, true);
            float value = parameter_manager_->getParameter(1003);
            
            // Should be quantized to nearest 0.01
            float remainder = std::fmod(value, 0.01f);
            ASSERT_TRUE(remainder < 0.001f || remainder > 0.009f);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 4: Parameter Smoothing
    void testParameterSmoothing() {
        TEST_SUITE("Parameter Smoothing");
        
        TEST("Parameter smoothing interpolates values") {
            parameter_manager_->resetStatistics();
            
            // Set initial value
            parameter_manager_->setParameter(1002, 1000.0f, true);
            float initial_value = parameter_manager_->getParameter(1002);
            
            // Set target value with smoothing
            parameter_manager_->setParameter(1002, 5000.0f, false);
            
            // Process smoothing several times
            for (int i = 0; i < 10; ++i) {
                parameter_manager_->processParameterSmoothing();
                float current_value = parameter_manager_->getParameter(1002);
                
                // Value should be moving toward target
                ASSERT_TRUE(current_value > initial_value);
                ASSERT_TRUE(current_value <= 5000.0f);
                
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            
            float final_value = parameter_manager_->getParameter(1002);
            ASSERT_TRUE(final_value > initial_value); // Should have moved
            
        } END_TEST();
        
        TEST("Immediate updates bypass smoothing") {
            // Set with immediate flag
            parameter_manager_->setParameter(4001, 0.5f, true);
            float value = parameter_manager_->getParameter(4001);
            
            // Should be set immediately
            ASSERT_TRUE(std::abs(value - 0.5f) < 0.001f);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 5: Thread Safety
    void testThreadSafety() {
        TEST_SUITE("Thread Safety");
        
        TEST("Concurrent parameter access is thread-safe") {
            const uint32_t param_id = 1002;
            const int num_threads = 4;
            const int operations_per_thread = 100;
            
            std::atomic<int> completed_threads{0};
            std::vector<std::thread> threads;
            
            // Launch reader threads
            for (int i = 0; i < num_threads / 2; ++i) {
                threads.emplace_back([&, param_id]() {
                    for (int j = 0; j < operations_per_thread; ++j) {
                        float value = parameter_manager_->getParameter(param_id);
                        (void)value; // Suppress unused variable warning
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                    }
                    completed_threads++;
                });
            }
            
            // Launch writer threads
            for (int i = 0; i < num_threads / 2; ++i) {
                threads.emplace_back([&, param_id, i]() {
                    for (int j = 0; j < operations_per_thread; ++j) {
                        float value = 1000.0f + (i * 100) + j;
                        parameter_manager_->setParameter(param_id, value, true);
                        std::this_thread::sleep_for(std::chrono::microseconds(15));
                    }
                    completed_threads++;
                });
            }
            
            // Wait for all threads
            for (auto& thread : threads) {
                thread.join();
            }
            
            ASSERT_EQ(num_threads, completed_threads.load());
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 6: RT Performance
    void testRTPerformance() {
        TEST_SUITE("RT Performance");
        
        TEST("Parameter access meets RT timing requirements") {
            parameter_manager_->resetStatistics();
            
            const int num_accesses = 1000;
            auto start_time = std::chrono::high_resolution_clock::now();
            
            // Perform many parameter accesses
            for (int i = 0; i < num_accesses; ++i) {
                float value1 = parameter_manager_->getParameter(1001);
                float value2 = parameter_manager_->getParameter(1002);
                float value3 = parameter_manager_->getParameter(4001);
                (void)value1; (void)value2; (void)value3; // Suppress warnings
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            
            uint32_t total_time_us = static_cast<uint32_t>(duration.count());
            float avg_time_per_access = static_cast<float>(total_time_us) / (num_accesses * 3);
            
            std::cout << "✅ (RT-safe, " << total_time_us << "μs total, " 
                      << std::fixed << std::setprecision(2) << avg_time_per_access << "μs avg)" << std::endl;
            
            // Parameter access should be very fast for RT use
            ASSERT_TRUE(avg_time_per_access < 10.0f); // Should be under 10μs per access
            
            // Check statistics
            auto stats = parameter_manager_->getStatistics();
            ASSERT_TRUE(stats.rt_access_count >= num_accesses * 3);
            ASSERT_TRUE(stats.max_access_time_us < 100); // No single access > 100μs
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 7: Parameter Categories and Types
    void testParameterCategories() {
        TEST_SUITE("Parameter Categories and Types");
        
        TEST("Parameters organized by category") {
            // Check that default parameters have correct categories
            const auto* osc_def = parameter_manager_->getParameterDefinition(1001);
            ASSERT_NE(nullptr, osc_def);
            ASSERT_EQ(RTSafeParameterManager::ParameterCategory::OSCILLATOR, osc_def->category);
            
            const auto* filter_def = parameter_manager_->getParameterDefinition(1002);
            ASSERT_NE(nullptr, filter_def);
            ASSERT_EQ(RTSafeParameterManager::ParameterCategory::FILTER, filter_def->category);
            
            const auto* master_def = parameter_manager_->getParameterDefinition(4001);
            ASSERT_NE(nullptr, master_def);
            ASSERT_EQ(RTSafeParameterManager::ParameterCategory::MASTER, master_def->category);
            
        } END_TEST();
        
        TEST("Parameter types and properties") {
            const auto* cutoff_def = parameter_manager_->getParameterDefinition(1002);
            ASSERT_NE(nullptr, cutoff_def);
            ASSERT_EQ(RTSafeParameterManager::ParameterType::CONTINUOUS, cutoff_def->type);
            ASSERT_TRUE(cutoff_def->logarithmic); // Frequency should be logarithmic
            ASSERT_TRUE(cutoff_def->automatable);
            ASSERT_TRUE(cutoff_def->real_time_safe);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 8: Event Notifications
    void testEventNotifications() {
        TEST_SUITE("Event Notifications");
        
        TEST("Parameter changes trigger events") {
            observer_->clearChanges();
            
            // Set multiple parameters
            parameter_manager_->setParameter(1001, 880.0f, true);  // Osc frequency
            parameter_manager_->setParameter(1002, 3000.0f, true); // Filter cutoff
            parameter_manager_->setParameter(4001, 0.7f, true);    // Master volume
            
            // Process any pending events
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // Check that all changes were observed
            ASSERT_TRUE(observer_->wasParameterChanged(1001));
            ASSERT_TRUE(observer_->wasParameterChanged(1002));
            ASSERT_TRUE(observer_->wasParameterChanged(4001));
            ASSERT_TRUE(observer_->getChangeCount() >= 3);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
    
    // Test 9: Statistics and Monitoring
    void testStatisticsAndMonitoring() {
        TEST_SUITE("Statistics and Monitoring");
        
        TEST("Statistics tracked correctly") {
            parameter_manager_->resetStatistics();
            observer_->clearChanges();
            
            // Perform various operations
            parameter_manager_->setParameter(1001, 220.0f, true);   // Set operation
            parameter_manager_->setParameter(1002, 4000.0f, false); // Set with smoothing
            
            float value1 = parameter_manager_->getParameter(1001);  // Get operation
            float value2 = parameter_manager_->getParameter(1002);  // Get operation
            (void)value1; (void)value2;
            
            // Process smoothing
            for (int i = 0; i < 3; ++i) {
                parameter_manager_->processParameterSmoothing();
            }
            
            auto stats = parameter_manager_->getStatistics();
            
            // Check statistics
            ASSERT_TRUE(stats.total_parameters > 0);
            ASSERT_TRUE(stats.parameter_changes >= 2);
            ASSERT_TRUE(stats.rt_access_count >= 2);
            ASSERT_TRUE(stats.max_access_time_us > 0);
            
        } END_TEST();
        
        END_TEST_SUITE();
    }
};

// Run the tests
int main() {
    std::cout << "🎛️  RT-Safe Parameter Manager Tests" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << "Testing the central parameter storage and management system" << std::endl;
    std::cout << "Built on the RT-Safe Event Distributor foundation" << std::endl << std::endl;
    
    RTSafeParameterManagerTests test_suite;
    return test_suite.run();
}
