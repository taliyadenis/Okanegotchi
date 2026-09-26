add_library(okanegachi_core STATIC
  src/nfc/ndef_parser.cpp
  src/core/animation_player.cpp
  src/core/button_input.cpp
  src/core/nfc_presence.cpp
  src/core/demo_tag_router.cpp
  src/network/json_codec.cpp
  src/app/app_engine.cpp
  src/app/fake_platform.cpp
)
target_include_directories(okanegachi_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/hardware
)
target_compile_definitions(okanegachi_core PUBLIC ARDUINOJSON_ENABLE_STD_STRING=1)
target_link_libraries(okanegachi_core PUBLIC ArduinoJson)
