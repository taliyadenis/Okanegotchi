cmake_minimum_required(VERSION 3.16)
project(okanegachi_firmware CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  ArduinoJson
  GIT_REPOSITORY https://github.com/bblanchon/ArduinoJson.git
  GIT_TAG v7.2.1
)
FetchContent_MakeAvailable(ArduinoJson)

add_subdirectory(firmware)

add_executable(firmware_test_driver tests/driver/firmware_test_driver.cpp)
target_link_libraries(firmware_test_driver PRIVATE okanegachi_core)

add_executable(content_test_driver tests/driver/content_test_driver.cpp)
target_link_libraries(content_test_driver PRIVATE okanegachi_core)

enable_testing()
add_test(NAME harness_smoke COMMAND harness_smoke)
add_executable(harness_smoke tests/support/harness_smoke.cpp)
target_include_directories(harness_smoke PRIVATE tests/support)
