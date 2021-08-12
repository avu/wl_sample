cmake_minimum_required(VERSION 3.20)
project(wl_sample)

set(CMAKE_CXX_STANDARD 14)

add_executable(display_connect display_connect.cpp)
target_link_libraries(display_connect wayland-client)

add_executable(display_create display_create.cpp)
target_link_libraries(display_create wayland-server)

add_executable(display_globals display_globals.cpp)
target_link_libraries(display_globals wayland-client)

add_executable(compositor compositor.cpp)
target_link_libraries(compositor wayland-client)
target_link_libraries(compositor rt)

add_executable(window window.cpp xdg-shell-protocol.c)
target_link_libraries(window wayland-client)
target_link_libraries(window rt)

add_executable(frame frame.cpp xdg-shell-protocol.c)
target_link_libraries(frame wayland-client)
target_link_libraries(frame rt)

add_executable(input_cap input_cap.cpp xdg-shell-protocol.c)
target_link_libraries(input_cap wayland-client)
target_link_libraries(input_cap rt)

add_executable(pnt_events pnt_events.cpp xdg-shell-protocol.c)
target_link_libraries(pnt_events wayland-client)
target_link_libraries(pnt_events rt)