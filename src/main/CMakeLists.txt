set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "/usr/bin")

include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(NOT WIN32)
    set(DST_DIRECTORY /tmp/stego_fuse/)
else()
    set(DST_DIRECTORY "$ENV{TEMP}/")
endif()

set(SRC_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/img)
set(PASSWORD "heslo")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)

if(NOT WIN32)
    set(LIBJPEGTURBO_LIBRARIES_STATIC ${CMAKE_BINARY_DIR}/${LIBDIR}/libjpeg.a)
else()
    link_directories(${CMAKE_BINARY_DIR}/${LIBDIR}/*/)
    set(LIBJPEGTURBO_LIBRARIES_STATIC jpeg-static)
endif()

if(FUSE_FOUND)
    add_executable(stego_fuse stego_fuse.cc)
    target_link_libraries(stego_fuse ${STEGODISK_LIBRARY} ${FUSE_LIBRARIES} ${LIBJPEGTURBO_LIBRARIES_STATIC})
endif()