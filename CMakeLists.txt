cmake_minimum_required(VERSION 3.16)

project(kmbasic VERSION 0.1 LANGUAGES C)

set(EXECUTABLE_OUTPUT_PATH ./${CMAKE_BUILD_TYPE})

set(INCLUDE_FILES
    common.h
    lexer.h
    parser.h
    tree.h
)

set(SRC_FILES
    main.c
    lexer.c
    parser.c
    tree.c
)

list(TRANSFORM INCLUDE_FILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/include/)
list(TRANSFORM SRC_FILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/src/)

add_executable(kmbasic ${INCLUDE_FILES} ${SRC_FILES})
target_include_directories(kmbasic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
