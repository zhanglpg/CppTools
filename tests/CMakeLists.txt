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

# Matrix and Kalman tests (no longer require Boost)
add_cpptools_test(test_matrixinversion test_matrixinversion.cpp)
add_cpptools_test(test_kalman          test_kalman.cpp)

# Tests for newly modernised utilities
add_cpptools_test(test_posixtime_util  test_posixtime_util.cpp)
add_cpptools_test(test_singleton       test_singleton.cpp)
add_cpptools_test(test_sleeprelative   test_sleeprelative.cpp)

# POSIX shared-memory IPC (links rt for shm_open/shm_unlink)
add_executable(test_interp test_interp.cpp)
target_link_libraries(test_interp PRIVATE commonlibs GTest::gtest_main)
if(UNIX)
    target_link_libraries(test_interp PRIVATE rt)
endif()
add_test(NAME test_interp COMMAND test_interp)
