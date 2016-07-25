cmake_minimum_required( VERSION 2.8.12 )

project( minimed_decode_prj )

include( ExternalProject )

find_package( Boost 1.54.0 COMPONENTS date_time system iostreams filesystem regex unit_test_framework REQUIRED )

if( ${CMAKE_CXX_COMPILER_ID} STREQUAL 'MSVC' )
	add_compile_options( -D_WIN32_WINNT=0x0601 ) 
else( )
	find_package( Threads REQUIRED )
	if( THREADS_HAVE_PTHREAD_ARG )
		set( COMPILER_SPECifIC_LIBS, "-lpthread" )
	endif( )

	if( ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" )
		add_compile_options(-std=c++14 -stdlib=libc++ -Weverything -Wno-c++98-compat -Wfatal-errors -Wno-covered-switch-default -Wno-padded -Wno-exit-time-destructors -Wno-c++98-compat-pedantic -Wno-unused-parameter -Wno-missing-noreturn -Wno-missing-prototypes -Wno-disabled-macro-expansion)		
	elseif( ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" )
		add_compile_options(-std=c++14 -Wall -Wno-deprecated-declarations)
	endif( )
endif( )

externalproject_add( 
	header_libraries_prj
	GIT_REPOSITORY "https://github.com/beached/header_libraries.git"
	GIT_TAG "master"
	SOURCE_DIR "${CMAKE_BINARY_DIR}/deps/header_libraries"
	INSTALL_DIR "${CMAKE_BINARY_DIR}/install"
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install
	PATCH_COMMAND ""
	TEST_COMMAND ""
 )

externalproject_add( 
	parse_json_prj
	GIT_REPOSITORY "https://github.com/beached/parse_json.git"
	GIT_TAG "master"
	SOURCE_DIR "${CMAKE_BINARY_DIR}/deps/parse_json"
	INSTALL_DIR "${CMAKE_BINARY_DIR}/install"
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install
	PATCH_COMMAND ""
	TEST_COMMAND ""
 )

externalproject_add( 
	char_rannge_prj
	GIT_REPOSITORY "https://github.com/beached/char_range.git"
	GIT_TAG "master"
	SOURCE_DIR "${CMAKE_BINARY_DIR}/deps/char_range"
	INSTALL_DIR "${CMAKE_BINARY_DIR}/install"
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/install
	PATCH_COMMAND ""
	TEST_COMMAND ""
 )



include_directories( SYSTEM "${CMAKE_BINARY_DIR}/install/include" )
link_directories( "${CMAKE_BINARY_DIR}/install/lib" )

include_directories( SYSTEM ${Boost_INCLUDE_DIRS} )
link_directories( ${Boost_LIBRARY_DIRS} )

set( HEADER_FOLDER "include" )
include_directories( ${HEADER_FOLDER} )

set( HEADER_FILES
	${HEADER_FOLDER}/history_pages_base.h
	${HEADER_FOLDER}/history_pages.h
	${HEADER_FOLDER}/bit.h
	${HEADER_FOLDER}/serial_port.h
)

set( SOURCE_FILES
	history_pages.cpp
	minimed_decode.cpp
)

add_executable( minimed_decode ${HEADER_FILES} ${SOURCE_FILES} )
add_dependencies( minimed_decode header_libraries_prj parse_json_prj char_rannge_prj )
target_link_libraries( minimed_decode char_range parse_json ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} )

add_executable( packet_decode ${HEADER_FILES} packet_decode.cpp )
add_dependencies( packet_decode header_libraries_prj parse_json_prj char_rannge_prj )
target_link_libraries( packet_decode char_range parse_json ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} )

add_executable( rfspy ${HEADER_FILES} rfspy.cpp serial_port.cpp )
add_dependencies( rfspy header_libraries_prj parse_json_prj char_rannge_prj )
target_link_libraries( rfspy char_range parse_json ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} )
