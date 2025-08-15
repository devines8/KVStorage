cmake_minimum_required(VERSION 3.14)
project(kv_storage VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Включение Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.11.0
)
FetchContent_MakeAvailable(googletest)

# Создаем интерфейсную библиотеку для заголовочного файла
add_library(kv_storage INTERFACE)
target_include_directories(kv_storage INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Тестовый исполняемый файл
add_executable(kv_storage_tests
  tests/test_kv_storage.cpp
)

target_link_libraries(kv_storage_tests
  PRIVATE
  kv_storage
  GTest::gtest_main
)

target_compile_features(kv_storage_tests PRIVATE cxx_std_20)

enable_testing()
add_test(NAME kv_storage_tests COMMAND kv_storage_tests)
