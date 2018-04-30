include_directories("${CMAKE_SOURCE_DIR}/include")
link_directories("${CMAKE_SOURCE_DIR}/redist")
add_executable(bot main.c)
target_link_libraries(bot crow requests curl json-c pthread ssl crypto nopoll)