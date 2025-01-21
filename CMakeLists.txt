cmake_minimum_required(VERSION 3.26)
project(TextRenderingWithoutTexture)

set(CMAKE_CXX_STANDARD 20)

set(APP_NAME TextRenderingWithoutTexture)

add_executable(${APP_NAME} main.c)
if(WIN32 AND MINGW)
    target_link_libraries(minimal_imgui PUBLIC kernel32 user32 shell32 gdi32)
elseif(APPLE)
    target_link_libraries(minimal_imgui PUBLIC "-framework Cocoa" "-framework QuartzCore" "-framework OpenGL")
elseif(UNIX AND NOT APPLE)
    target_link_libraries(minimal_imgui PUBLIC X11 Xi Xcursor GL dl pthread m)
endif()
