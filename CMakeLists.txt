
cmake_minimum_required(VERSION 3.5)

project(ratfist VERSION 0.0.1 LANGUAGES C)

find_package(Doxygen)

if(NOT DEFINED CHIP_FAMILY)
    message(FATAL_ERROR "A STM32 chip family must be defined. -DCHIP_FAMILY=...\nAllowed families are: STM32F0")
endif()


if(CHIP_FAMILY STREQUAL "STM32F0")
    set(BOARD_FILE "/usr/share/openocd/scripts/board/stm32f0discovery.cfg")
    set(LINK_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/ld/stm32f072rb.ld")
    set(BSP_SRC_FILE "${CMAKE_CURRENT_LIST_DIR}/src/stm32f072-discovery/bsp.c")
else()
    message(FATAL_ERROR "Unknown CHIP_FAMILY: ${CHIP_FAMILY}\nAllowed families are: STM32F0")
endif()



add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/libsrc/mouros")


add_executable(${PROJECT_NAME}
    "${CMAKE_CURRENT_LIST_DIR}/src/main.c"
    "${BSP_SRC_FILE}"
    "${CMAKE_CURRENT_LIST_DIR}/src/constants.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/messages.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/messages.c"
)


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

target_link_libraries(${PROJECT_NAME} mouros)


set_property(
    TARGET ${PROJECT_NAME}
    PROPERTY LINK_FLAGS "-T ${LINK_SCRIPT} -Wl,-lc -Wl,--gc-sections"
)

if(NOT USE_FULL_NEWLIB)
    set_property(
        TARGET ${PROJECT_NAME}
        PROPERTY LINK_FLAGS " --specs=nano.specs"
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

    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doc" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}")
else()
    message(WARNING "Doxygen or dot missing. Won't create doc target.")
endif()





install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
)

install(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
)
