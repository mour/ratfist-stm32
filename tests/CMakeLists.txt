cmake_minimum_required(VERSION 3.5)

project(ratfist-tests VERSION 0.0.1 LANGUAGES C)



include(ExternalProject)

get_filename_component(TOOLCHAIN_FILE_ABSPATH "${CMAKE_TOOLCHAIN_FILE}" ABSOLUTE)

# Build cmocka
ExternalProject_Add(cmocka
    URL https://cmocka.org/files/1.0/cmocka-1.0.1.tar.xz
    URL_MD5 ed861e501a21a92b2af63e466df2015e
    CMAKE_ARGS "$<$<BOOL:${CMAKE_TOOLCHAIN_FILE}>:-DCMAKE_TOOLCHAIN_FILE:string=${TOOLCHAIN_FILE_ABSPATH}>"
    CMAKE_ARGS "$<$<BOOL:${CMAKE_INSTALL_PREFIX}>:-DCMAKE_INSTALL_PREFIX:string=${CMAKE_INSTALL_PREFIX}>"
)


enable_testing()

# Link cmocka
link_libraries("${CMAKE_BINARY_DIR}/cmocka-prefix/src/cmocka-build/src/libcmocka.so")

# Enable coverage
link_libraries("--coverage")

# Add include stub overrides
include_directories(
    "${CMAKE_CURRENT_LIST_DIR}/../libsrc/mouros/tests/stubs/include"
    "${CMAKE_CURRENT_LIST_DIR}/stubs/include"
)

# Add public include dirs
include_directories(SYSTEM
    "${CMAKE_INSTALL_PREFIX}/include"
)

# Add a few warnings
add_compile_options(
    "-std=gnu11"
    "-Wunused"
    "-Wuninitialized"
    "-Wall"
    "-Wextra"
    "-Wmissing-declarations"
    "-Wconversion"
    "-Wpointer-arith"
    "-Wshadow"
    "-Wlogical-op"
    "-Waggregate-return"
    "-Wfloat-equal"
    "$<$<CONFIG:Debug>:-g3>"
)



# Spinner tests
add_executable(test_spinner
    "${CMAKE_CURRENT_LIST_DIR}/../src/spinner/spinner.h"
    "${CMAKE_CURRENT_LIST_DIR}/../src/spinner/spinner.c"
    "${CMAKE_CURRENT_LIST_DIR}/test_spinner.c"
    "${CMAKE_CURRENT_LIST_DIR}/../libsrc/mouros/src/pool_alloc.c"
    "${CMAKE_CURRENT_LIST_DIR}/../libsrc/mouros/src/mailbox.c"
    "${CMAKE_CURRENT_LIST_DIR}/stubs/ratfist/worker.c"
    "${CMAKE_CURRENT_LIST_DIR}/stubs/ratfist/message_dispatcher.c"
)

set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/../src/spinner/spinner.c" PROPERTIES COMPILE_FLAGS "--coverage")

add_test(NAME spinner COMMAND test_spinner)
set_tests_properties(spinner PROPERTIES DEPENDS test_spinner)

add_dependencies(test_spinner cmocka)



# Message dispatcher tests
add_executable(test_dispatcher
    "${CMAKE_CURRENT_LIST_DIR}/../src/message_dispatcher.h"
    "${CMAKE_CURRENT_LIST_DIR}/../src/message_dispatcher.c"
    "${CMAKE_CURRENT_LIST_DIR}/test_dispatcher.c"
    "${CMAKE_CURRENT_LIST_DIR}/../libsrc/mouros/src/mailbox.c"
    "${CMAKE_CURRENT_LIST_DIR}/../libsrc/mouros/src/char_buffer.c"
    "${CMAKE_CURRENT_LIST_DIR}/../libsrc/mouros/tests/stubs/mouros/tasks.c"
    "${CMAKE_CURRENT_LIST_DIR}/stubs/ratfist/worker.c"
)

set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/../src/message_dispatcher.c" PROPERTIES COMPILE_FLAGS "--coverage")

add_test(NAME dispatcher COMMAND test_dispatcher)
set_tests_properties(dispatcher PROPERTIES DEPENDS test_dispatcher)

add_dependencies(test_dispatcher cmocka)


# Covearge
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/coverage")

add_custom_target(coverage
    COMMAND gcovr -r "${CMAKE_CURRENT_LIST_DIR}/../src" --html --html-details -b -o coverage/${PROJECT_NAME}-coverage.html .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)

