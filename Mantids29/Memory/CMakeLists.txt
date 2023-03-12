cmake_minimum_required(VERSION 3.10)

get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
get_filename_component(PARENT_DIRFULL ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
get_filename_component(PARENT_DIR ${PARENT_DIRFULL} NAME)

SET(RLIB_NAME ${LIB_NAME})
SET(LIB_NAME ${LIBPREFIX}_${LIB_NAME})

project(${LIB_NAME})
project(${LIB_NAME} VERSION ${SVERSION} DESCRIPTION "Mantids29 Libraries for Abstract Variables Library")

file(GLOB_RECURSE EDV_INCLUDE_FILES "./*.h*")
file(GLOB_RECURSE EDV_SOURCE_FILES "./*.c*")

add_library(${LIB_NAME} ${EDV_INCLUDE_FILES} ${EDV_SOURCE_FILES})



set_target_properties(  ${LIB_NAME}
                        PROPERTIES VERSION ${PROJECT_VERSION}
                        SOVERSION 2
                        PUBLIC_HEADER "${EDV_INCLUDE_FILES}"
                      )

configure_file( "genericlib.pc.in" "${LIB_NAME}.pc" @ONLY )

target_include_directories(${LIB_NAME} PRIVATE .)

install(
        TARGETS ${LIB_NAME}
        ARCHIVE COMPONENT lib DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY COMPONENT lib DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER COMPONENT dev DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIBPREFIX}/${RLIB_NAME}
        )

install(
        FILES ${CMAKE_BINARY_DIR}/${PARENT_DIR}/${RLIB_NAME}/${LIB_NAME}.pc
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig
        )

if (EXTRAPREFIX)
    target_include_directories(${LIB_NAME} PUBLIC ${EXTRAPREFIX}/include)
    target_link_libraries(${LIB_NAME} "-L${EXTRAPREFIX}/lib")
endif()

find_library (
    WINDOWS_SOCKLIB
    NAMES ws2_32 libws2_32 # what to look for
)

if (WINDOWS_SOCKLIB)
    message("-- WINDOWS_SOCKLIB found at [${WINDOWS_SOCKLIB}] for ${LIB_NAME}")
    target_link_libraries(${LIB_NAME} ${WINDOWS_SOCKLIB})
elseif(WIN32)
    # AT LEAST PASS IT TO THE COMPILER...
    message("-- Using native WINDOWS_SOCKLIB")
    target_link_libraries(${LIB_NAME} "-lws2_32")
endif()

include_directories("${Mantids29_Helpers_SOURCE_DIR}/../../")
target_link_libraries(${LIB_NAME} "${LIBPREFIX}_Helpers")

include_directories("${Mantids29_Threads_SOURCE_DIR}/../../")
target_link_libraries(${LIB_NAME} "${LIBPREFIX}_Threads")
message("-- TRG LIB LIBRARY: ${Mantids29_Threads_SOURCE_DIR}")