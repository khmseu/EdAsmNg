find_package(Curses REQUIRED)

add_library(EdAsmNg_lib
  lib/app.cpp
  lib/ei/relocator.cpp
)

target_include_directories(EdAsmNg_lib PUBLIC
  ${PROJECT_SOURCE_DIR}/include
  ${CURSES_INCLUDE_DIR}
)

target_compile_features(EdAsmNg_lib PUBLIC cxx_std_17)

if(MSVC)
  target_compile_options(EdAsmNg_lib PRIVATE /W4)
else()
  target_compile_options(EdAsmNg_lib PRIVATE -Wall -Wextra -Wpedantic)
  if(ENABLE_WARNINGS_AS_ERRORS)
    target_compile_options(EdAsmNg_lib PRIVATE -Werror)
  endif()
endif()

add_executable(EdAsmNg_app
  main.cpp
)

target_link_libraries(EdAsmNg_lib PUBLIC ${CURSES_LIBRARIES})

target_link_libraries(EdAsmNg_app PRIVATE EdAsmNg_lib)
