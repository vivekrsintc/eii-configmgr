# Copyright (c) 2020 Intel Corporation.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

set(CMAKE_CXX_STANDARD 11)
configure_file(${EII_COMMON_CMAKE}/GoogleTestCMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} --build .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build)

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
    include_directories("${gtest_SOURCE_DIR}/include")
endif()

# Now simply link against gtest or gtest_main as needed. Eg
add_executable(config_manager_unit_tests "config_manager_unit_tests.cpp")
add_executable(cfgmgr_c_apis_unit_tests "cfgmgr_c_apis_unit_tests.cpp")
add_executable(kvstore_client-tests "kv_store_client_tests.cpp")
target_link_libraries(config_manager_unit_tests eiiconfigmanager eiimsgbus eiimsgenv cjson eiiutils gtest_main eiiutils)
target_link_libraries(cfgmgr_c_apis_unit_tests eiiconfigmanager eiimsgbus eiimsgenv cjson eiiutils gtest_main eiiutils)
target_link_libraries(kvstore_client-tests eiiconfigmanager gtest_main eiiutils)
add_test(NAME config_manager_unit_tests COMMAND config_manager_unit_tests)
add_test(NAME kvstore_client-tests COMMAND kvstore_client-tests)

# Copy JSON configuration for unit-tests
#file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/kv_store_config.json"
#     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

# Copy JSON configuration for unit-tests
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/kv_store_unittest_config.json"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
