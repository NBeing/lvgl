# CMake Integration for Unified Test Framework
# Add this to your main CMakeLists.txt to enable testing

cmake_minimum_required(VERSION 3.16)

# Enable testing
enable_testing()

# Test configuration
set(TEST_FRAMEWORK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test/framework")
set(TEST_FIXTURES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test/fixtures")
set(TEST_SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test")

# Include directories for tests
include_directories(${TEST_FRAMEWORK_DIR})
include_directories(${TEST_FIXTURES_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Compiler flags for tests
set(TEST_COMPILE_FLAGS "-std=c++17 -Wall -Wextra -g -O0 -DDESKTOP_BUILD=1")

# Function to create a test executable
function(add_unified_test test_name test_source)
    add_executable(${test_name} ${test_source})
    target_compile_options(${test_name} PRIVATE ${TEST_COMPILE_FLAGS})
    
    # Link threading library if needed
    find_package(Threads REQUIRED)
    target_link_libraries(${test_name} Threads::Threads)
    
    # Add the test to CTest
    add_test(NAME ${test_name} COMMAND ${test_name})
    
    # Set test properties
    set_tests_properties(${test_name} PROPERTIES
        TIMEOUT 30
        WILL_FAIL FALSE
    )
endfunction()

# Define all test executables
add_unified_test(parameter_lock_test 
    "${TEST_SOURCES_DIR}/examples/improved_parameter_lock_test.cpp")

add_unified_test(event_tracer_test 
    "${TEST_SOURCES_DIR}/unit/event_tracer_test_fixed.cpp")

add_unified_test(midi_system_test 
    "${TEST_SOURCES_DIR}/unit/midi_system_test_fixed.cpp")

add_unified_test(bidirectional_bridge_test 
    "${TEST_SOURCES_DIR}/integration/bidirectional_bridge_test.cpp")

# Custom target to run all tests
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS parameter_lock_test event_tracer_test midi_system_test bidirectional_bridge_test
    COMMENT "Running all unified tests"
)

# Custom target to run only unit tests
add_custom_target(run_unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R ".*unit.*"
    DEPENDS event_tracer_test midi_system_test
    COMMENT "Running unit tests only"
)

# Custom target to run only integration tests
add_custom_target(run_integration_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R ".*integration.*"
    DEPENDS bidirectional_bridge_test
    COMMENT "Running integration tests only"
)

# Custom target for test coverage (if gcov/lcov available)
find_program(GCOV_PATH gcov)
find_program(LCOV_PATH lcov)
find_program(GENHTML_PATH genhtml)

if(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
    add_custom_target(test_coverage
        COMMAND ${CMAKE_COMMAND} -E make_directory coverage
        COMMAND ${LCOV_PATH} --directory . --zerocounters
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage/coverage.info
        COMMAND ${LCOV_PATH} --remove coverage/coverage.info '/usr/*' '*/test/*' --output-file coverage/coverage.cleaned.info
        COMMAND ${GENHTML_PATH} -o coverage coverage/coverage.cleaned.info
        COMMAND ${CMAKE_COMMAND} -E echo "Coverage report generated in coverage/index.html"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating test coverage report"
    )
endif()

# Print test summary
message(STATUS "Unified Test Framework Configuration:")
message(STATUS "  Framework Dir: ${TEST_FRAMEWORK_DIR}")
message(STATUS "  Fixtures Dir:  ${TEST_FIXTURES_DIR}")
message(STATUS "  Test Sources:  ${TEST_SOURCES_DIR}")
message(STATUS "  Available targets:")
message(STATUS "    make run_all_tests")
message(STATUS "    make run_unit_tests") 
message(STATUS "    make run_integration_tests")
if(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
    message(STATUS "    make test_coverage")
endif()
