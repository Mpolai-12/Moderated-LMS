cmake_minimum_required(VERSION 3.16)
project(ModeratedLMS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(LMS_BUILD_TESTS "Build the automated tests" ON)
option(LMS_COVERAGE "Instrument the build for gcov/gcovr coverage" OFF)

find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED COMPONENTS Crypto)

# One place for the warning policy (see docs/CODING_STANDARD.md).
function(lms_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror)
  endif()
  if(LMS_COVERAGE AND NOT MSVC)
    target_compile_options(${target} PRIVATE --coverage -O0 -g)
    target_link_options(${target} PUBLIC --coverage)
  endif()
endfunction()

# ---- domain layer: pure C++, no SQLite, no console -------------------------
add_library(lms_domain STATIC
  src/domain/Types.cpp
  src/domain/Time.cpp
  src/domain/User.cpp
  src/domain/UserFactory.cpp
  src/domain/Entities.cpp
  src/domain/TieredDailyPenalty.cpp
  src/domain/MarkCalculator.cpp
  src/domain/Submission.cpp)
target_include_directories(lms_domain PUBLIC src)
lms_warnings(lms_domain)

# Console application: SQLite persistence and PBKDF2 password hashing via OpenSSL.
add_executable(lms_app src/app/main.cpp)
target_link_libraries(lms_app PRIVATE lms_domain SQLite::SQLite3 OpenSSL::Crypto)
lms_warnings(lms_app)

if(LMS_BUILD_TESTS)
  enable_testing()
  find_package(Catch2 3 QUIET)
  if(NOT Catch2_FOUND)
    include(FetchContent)
    FetchContent_Declare(Catch2 GIT_REPOSITORY https://github.com/catchorg/Catch2.git GIT_TAG v3.5.2)
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
  endif()
  add_executable(lms_tests
    tests/test_time.cpp
    tests/test_penalty.cpp
    tests/test_submission.cpp
    tests/test_users.cpp)
  target_link_libraries(lms_tests PRIVATE lms_domain Catch2::Catch2WithMain)
  lms_warnings(lms_tests)
  # Keep test discovery out of the build step: some Windows/MSYS2 setups
  # cannot launch Catch2 discovery automatically. CTest runs the full suite.
  add_test(NAME lms_unit_tests COMMAND lms_tests)
endif()
