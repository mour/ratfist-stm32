
cmake_minimum_required(VERSION 3.5)

project(ratfist VERSION 0.2.0 LANGUAGES C)

find_package(Doxygen)

if(NOT DEFINED BOARD_TYPE)
    message(FATAL_ERROR "A STM32 board type must be defined. -DBOARD_TYPE=...\nAllowed board types are: stm32f072discovery, stm32f411discovery")
endif()


if(BOARD_TYPE STREQUAL "stm32f072discovery")
    set(BOARD_FILE "/usr/share/openocd/scripts/board/stm32f0discovery.cfg")
    set(LINK_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/ld/stm32f072rb.ld")
    set(XARGO_TARGET "thumbv6m-none-eabi")
elseif(BOARD_TYPE STREQUAL "stm32f411discovery")
    set(BOARD_FILE "/usr/share/openocd/scripts/board/stm32f4discovery.cfg")
    set(LINK_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/ld/stm32f411ve.ld")
    set(XARGO_TARGET "thumbv7em-none-eabihf")
else()
    message(FATAL_ERROR "Unknown BOARD_TYPE: ${BOARD_TYPE}\nAllowed board types are: stm32f072discovery, stm32f411discovery")
endif()


# MourOS
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/libsrc/mouros")


# Rust part of ratfist
add_custom_target(rust-lib
                  "xargo" "build" "--lib"
                  "--target" "${XARGO_TARGET}"
                  "$<$<CONFIG:Release>:--release>"
                  "$<$<OR:${INCLUDE_SPINNER}>:--features>"
                  "$<$<BOOL:${INCLUDE_SPINNER}>:spinner>"
                  WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/rust")

add_custom_target(rust-clean "xargo" "clean"
                  WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/rust")



set(SPINNER_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/src/spinner/spinner.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/spinner/spinner.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/spinner/constants.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/spinner/bsp.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/spinner/${BOARD_TYPE}/bsp.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/spinner/${BOARD_TYPE}/constants.h"
)

# Ratfist binary
add_executable(${PROJECT_NAME}
    "${CMAKE_CURRENT_LIST_DIR}/src/main.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/${BOARD_TYPE}/bsp.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/constants.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/message_dispatcher.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/message_dispatcher.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/worker.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/worker.c"

    "$<$<BOOL:${INCLUDE_SPINNER}>:${SPINNER_SOURCES}>"
)

add_dependencies(${PROJECT_NAME} rust-lib)


target_compile_options(${PROJECT_NAME}
    PRIVATE "-Og"
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
            "$<$<CONFIG:Debug>:-g3>")


target_compile_definitions(${PROJECT_NAME} PUBLIC
    "$<UPPER_CASE:${BOARD_TYPE}>"
    "$<$<BOOL:${INCLUDE_SPINNER}>:INCLUDE_SPINNER>"
)


target_link_libraries(${PROJECT_NAME} m)
target_link_libraries(${PROJECT_NAME} mouros)
target_link_libraries(${PROJECT_NAME} "${CMAKE_CURRENT_LIST_DIR}/rust/target/${XARGO_TARGET}/$<LOWER_CASE:${CMAKE_BUILD_TYPE}>/libratfist.a")


set_property(
    TARGET ${PROJECT_NAME}
    PROPERTY LINK_FLAGS "-T ${LINK_SCRIPT} -Wl,-lc -Wl,--gc-sections"
)

if(NOT USE_FULL_NEWLIB)
    # Also pull in floating point support for *printf functions.
    # However, during linking this causes _sbrk_r to be pulled in from newlib
    # nano, instead of the _sbrk_r implementation from libmouros. To get around
    # this, newlib nano must be compiled with -DREENTRANT_SYSCALLS_PROVIDED.
    set_property(
        TARGET ${PROJECT_NAME}
        PROPERTY LINK_FLAGS " --specs=nano.specs -Wl,--require-defined=_printf_float"
        APPEND_STRING
    )
endif()



set_source_files_properties(${PROJECT_NAME} PROPERTIES GENERATED TRUE)

add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin"
                   COMMAND "${TARGET_TRIPLET}-objcopy" "-Obinary" "${CMAKE_BINARY_DIR}/${PROJECT_NAME}" "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin"
                   MAIN_DEPENDENCY ${PROJECT_NAME})

add_custom_target(create_binary_image ALL DEPENDS "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin")



add_custom_target(upload
    "openocd" "-f" "${BOARD_FILE}"
              "-c" "init" "-c" "targets" "-c" "reset halt"
              "-c" "flash write_image erase ratfist.bin 0x08000000"
              "-c" "verify_image ratfist.bin"
              "-c" "reset run" "-c" "shutdown"
    DEPENDS "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin")




add_custom_target(show_size ALL "${TARGET_TRIPLET}-size" "${CMAKE_BINARY_DIR}/${PROJECT_NAME}")
add_dependencies(show_size ${PROJECT_NAME})





if(DOXYGEN_FOUND AND DOXYGEN_DOT_FOUND)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/doc/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    add_custom_target(${PROJECT_NAME}-doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    )

    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doc" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}" OPTIONAL)
else()
    message(WARNING "Doxygen or dot missing. Won't create doc target.")
endif()





install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
)

install(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
)
