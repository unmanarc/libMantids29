cmake_minimum_required(VERSION 3.10)

get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
get_filename_component(PARENT_DIRFULL ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
get_filename_component(PARENT_DIR ${PARENT_DIRFULL} NAME)

SET(RLIB_NAME ${LIB_NAME})
SET(LIB_NAME ${LIBPREFIX}_${LIB_NAME})

project(${LIB_NAME})
project(${LIB_NAME} VERSION ${SVERSION} DESCRIPTION "Mantids29 Libraries for XRPC Webserver Library")

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

include_directories("${Mantids29_Threads_SOURCE_DIR}/../../")
target_link_libraries(${LIB_NAME} ${LIBPREFIX}_Threads)

include_directories("${Mantids29_Helpers_SOURCE_DIR}/../../")
target_link_libraries(${LIB_NAME} ${LIBPREFIX}_Helpers)

include_directories("${Mantids29_Auth_SOURCE_DIR}/../../")
target_link_libraries(${LIB_NAME} ${LIBPREFIX}_Auth)

include_directories("${Mantids29_Memory_SOURCE_DIR}/../../")
target_link_libraries(${LIB_NAME} ${LIBPREFIX}_Memory)

include_directories("${Mantids29_Net_Sockets_SOURCE_DIR}/../../")
target_link_libraries(${LIB_NAME} ${LIBPREFIX}_Net_Sockets)

include_directories("${Mantids29_Protocol_HTTP_SOURCE_DIR}/../../")
target_link_libraries(${LIB_NAME} ${LIBPREFIX}_Protocol_HTTP)

include_directories("${Mantids29_Protocol_MIME_SOURCE_DIR}/../../")
target_link_libraries(${LIB_NAME} ${LIBPREFIX}_Protocol_MIME)

include_directories("${Mantids29_RPC_Common_SOURCE_DIR}/../../")
target_link_libraries(${LIB_NAME} ${LIBPREFIX}_RPC_Common)

include_directories("${Mantids29_Program_Logs_SOURCE_DIR}/../../")
target_link_libraries(${LIB_NAME} ${LIBPREFIX}_Program_Logs)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)
link_libraries(${JSONCPP_LIBRARIES})
target_include_directories(${LIB_NAME} PUBLIC ${JSONCPP_INCLUDE_DIRS})
target_link_libraries(${LIB_NAME} ${JSONCPP_LIBRARIES})



find_library (
    BOOST_REGEX_LIB
    NAMES boost_regex-mt-x32 libboost_regex-mt-x32 boost_regex-mt libboost_regex-mt boost_regex libboost_regex # what to look for
)
if (NOT BOOST_REGEX_LIB)
    message("ERROR: Did not found lib BOOST_REGEX_LIB")
else()
    message("-- BOOST_REGEX_LIB found at [${BOOST_REGEX_LIB}] for ${LIB_NAME}")
endif()

target_link_libraries(${LIB_NAME} ${BOOST_REGEX_LIB})



