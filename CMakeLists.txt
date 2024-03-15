# CMakeList.txt : CMake project for DNSCache, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("DNSCache")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
find_package(Threads REQUIRED)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable (DNSCache "main.cpp" "DNSCache.cpp" "DNSCache.h")
target_link_libraries(DNSCache ${CMAKE_THREAD_LIBS_INIT})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET DNSCache PROPERTY CXX_STANDARD 17)
endif()

enable_testing()

add_executable(DNSCacheTest "tests.cpp" "DNSCache.cpp" "DNSCache.h")
target_link_libraries(DNSCacheTest ${CMAKE_THREAD_LIBS_INIT} GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(DNSCacheTest)