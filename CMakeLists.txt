cmake_minimum_required(VERSION 3.28)
project(SackServer LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(sackServ
    src/main.c
    src/libserv.c
    src/sockwrap.c
    src/Queue.c
    src/roomOp.c
    src/gameplay.c
    src/errfunc.c
)

add_executable(apServ
    src/apServ.c
)

if(WIN32)
    target_link_libraries(sackServ ws2_32)
    target_link_libraries(apServ ws2_32)
    
    target_compile_definitions(sackServ PRIVATE _WIN32)
    target_compile_definitions(apServ PRIVATE _WIN32)

    add_custom_command(TARGET sackServ POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:sackServ> ${CMAKE_SOURCE_DIR}/$<TARGET_FILE_NAME:sackServ>
        COMMENT "Copying sackServ to root directory"
    )

    add_custom_command(TARGET apServ POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:apServ> ${CMAKE_SOURCE_DIR}/$<TARGET_FILE_NAME:apServ>
        COMMENT "Copying apServ to root directory"
    )
else()
    add_custom_command(TARGET sackServ POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/out
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:sackServ> ${CMAKE_SOURCE_DIR}/out/$<TARGET_FILE_NAME:sackServ>
        COMMENT "Copying sackServ to ./out"
    )

    add_custom_command(TARGET apServ POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/out
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:apServ> ${CMAKE_SOURCE_DIR}/out/$<TARGET_FILE_NAME:apServ>
        COMMENT "Copying apServ to ./out"
    )
endif()
