#include "../framework/unified_test_framework.h"
#include <stdexcept>

TEST_UNIT(Example, FailingTest) {
    PRINT_TEST_HEADER("Intentional Failing Test");
    
    // Test assertion failure
    ASSERT_TRUE(false, "This assertion should fail");
    
    PASS("This should never execute");
}

TEST_UNIT(Example, ThrowingTest) {
    PRINT_TEST_HEADER("Test that throws exception");
    
    // Test unhandled exception
    throw std::runtime_error("Intentional runtime error with detailed message");
    
    PASS("This should never execute");
}

TEST_UNIT(Example, PassingTest) {
    PRINT_TEST_HEADER("This test should pass");
    
    ASSERT_TRUE(true, "This should pass");
    
    PASS("This test passes correctly");
}

int main() {
    return RUN_ALL_TESTS();
}
