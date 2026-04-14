include(FetchContent)

# Avoid overriding parent project compiler/linker settings in GoogleTest on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(googletest)

add_executable(EdAsmNg_app_test
  app_test.cpp
)

target_link_libraries(EdAsmNg_app_test PRIVATE
  EdAsmNg_lib
  GTest::gtest_main
)

target_compile_definitions(EdAsmNg_app_test PRIVATE
  EDASMNG_APP_PATH="$<TARGET_FILE:EdAsmNg_app>"
)

if(MSVC)
  target_compile_options(EdAsmNg_app_test PRIVATE /W4)
else()
  target_compile_options(EdAsmNg_app_test PRIVATE -Wall -Wextra -Wpedantic)
  if(ENABLE_WARNINGS_AS_ERRORS)
    target_compile_options(EdAsmNg_app_test PRIVATE -Werror)
  endif()
endif()

include(GoogleTest)
gtest_discover_tests(EdAsmNg_app_test)
