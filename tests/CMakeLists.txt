# Helper to register a test executable
function(add_cpptools_test target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE commonlibs GTest::gtest_main)
    add_test(NAME ${target} COMMAND ${target})
endfunction()

# Tests that depend only on the C++ standard library
add_cpptools_test(test_segment_tree  test_segment_tree.cpp)
add_cpptools_test(test_prefix_tree   test_prefix_tree.cpp)
add_cpptools_test(test_algorithms    test_algorithms.cpp)
add_cpptools_test(test_errorstatus   test_errorstatus.cpp)

# Tests that require Boost uBLAS
if(Boost_FOUND)
    add_cpptools_test(test_matrixinversion test_matrixinversion.cpp)
    add_cpptools_test(test_kalman          test_kalman.cpp)
endif()
