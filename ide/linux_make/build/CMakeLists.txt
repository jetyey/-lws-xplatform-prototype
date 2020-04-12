cmake_minimum_required(VERSION 2.8)
project(lws-test-server)

add_compile_definitions(_LINUX_SOURCE)

FIND_PATH ( LIBWEBSOCKETS_INCLUDE_DIR libwebsockets.h
    ../../lib/libwebsockets/linux_a64/include
)

#FIND_LIBRARY ( LIBWEBSOCKETS_LIBRARIES websockets.a
#    ../../lib/libwebsockets/linux_a64/lib
#)

add_library( lws-test-server SHARED IMPORTED GLOBAL )

#GET_FILENAME_COMPONENT( LIBWEBSOCKETS_LIBRARY_DIR ${LIBWEBSOCKETS_LIBRARIES} PATH )

add_executable(lws-test-server ../../dev/jds19/my-lws-server.cpp)

#target_link_libraries(lws-test-server "${LIBWEBSOCKETS_LIBRARIES}")

include_directories(lws-test-server "${LIBWEBSOCKETS_INCLUDE_DIR}")

set_target_properties(
    lws-test-server
    PROPERTIES
	IMPORTED_LOCATION
		../../lib/libwebsockets/linux_a64/lib/websockets.so
)


