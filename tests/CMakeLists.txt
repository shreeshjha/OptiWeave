# OptiWeave Test Suite

# Find GTest
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    # Download and build GTest
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50d33092f97b05ab7be1d1027d0efc.zip
        URL_HASH SHA256=ad7fdba11ea011c1d925b3289cf4af2c66a352e18d4c7264392fead75e919363
    )
    
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Helper function to add tests
function(add_optiweave_test test_name)
    add_executable(${test_name} ${ARGN})
    target_link_libraries(${test_name} 
        PRIVATE 
        optiweave_core
        gtest_main
        gtest
    )
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    # Add test to CTest
    add_test(NAME ${test_name} COMMAND ${test_name})
    
    # Set test properties
    set_tests_properties(${test_name} PROPERTIES
        TIMEOUT 60
        LABELS "unit"
    )
endfunction()

# Unit tests
add_optiweave_test(test_ast_visitor unit/test_ast_visitor.cpp)
add_optiweave_test(test_rewriter unit/test_rewriter.cpp)
add_optiweave_test(test_operator_detection unit/test_operator_detection.cpp)
add_optiweave_test(test_template_handling unit/test_template_handling.cpp)

# Integration tests
add_optiweave_test(test_array_subscript_integration integration/test_array_subscript.cpp)
add_optiweave_test(test_arithmetic_integration integration/test_arithmetic_ops.cpp)
add_optiweave_test(test_end_to_end integration/test_end_to_end.cpp)

# Custom test target for running specific test categories
add_custom_target(test_unit
    COMMAND ${CMAKE_CTEST_COMMAND} -L "unit" --output-on-failure
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running unit tests"
)

add_custom_target(test_integration
    COMMAND ${CMAKE_CTEST_COMMAND} -L "integration" --output-on-failure
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running integration tests"
)

# Add test fixtures directory to install
install(DIRECTORY fixtures/
    DESTINATION share/optiweave/test_fixtures
    FILES_MATCHING PATTERN "*.cpp" PATTERN "*.hpp" PATTERN "*.h"
)
