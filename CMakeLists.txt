cmake_minimum_required(VERSION 3.2)
project(athena)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include( CTest )
include( version )
include( detectCacheSize )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_C_FLAGS_NOPANTS "${CMAKE_C_FLAGS_NOPANTS} -O3 -DNDEBUG -DAthena_DISABLE_VALIDATION")

include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR}/ccnx/forwarder/athena)

include_directories($ENV{CCNX_DEPENDENCIES}/include)
set(OPENSSL_ROOT_DIR $ENV{CCNX_DEPENDENCIES})

find_package( LongBow REQUIRED )
include_directories(${LONGBOW_INCLUDE_DIRS})

find_package( LibEvent REQUIRED )
include_directories(${LIBEVENT_INCLUDE_DIRS})

find_package( Libparc REQUIRED )
include_directories(${LIBPARC_INCLUDE_DIRS})

find_package( CCNX_Common REQUIRED )
include_directories(${CCNX_COMMON_INCLUDE_DIRS})

find_package( CCNX_Transport_Rta REQUIRED )
include_directories(${CCNX_TRANSPORT_RTA_INCLUDE_DIRS})

find_package( CCNX_Portal REQUIRED )
include_directories(${CCNX_PORTAL_INCLUDE_DIRS})

find_package ( Threads REQUIRED )

find_package ( OpenSSL REQUIRED )

find_package( Doxygen )

set(ATHENA_LINK_LIBRARIES
  athena
  ${LONGBOW_LIBRARIES}
  ${LIBEVENT_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${CCNX_PORTAL_LIBRARIES}
  ${CCNX_TRANSPORT_RTA_LIBRARIES}
  ${CCNX_COMMON_LIBRARIES}
  ${LIBPARC_LIBRARIES}
  dl
  )

if( UNIX )
  link_libraries(m)
endif( UNIX )

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

macro(AddTest testFile)
  add_executable(${ARGV0} ${ARGV0}.c)
  target_link_libraries(${ARGV0} ${ATHENA_LINK_LIBRARIES})
  add_test(${ARGV0} ${ARGV0})
  set_target_properties(${ARGV0} PROPERTIES FOLDER Test)
  set_tests_properties(${ARGV0} PROPERTIES ENVIRONMENT LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/ccnx/forwarder/athena)
endmacro(AddTest)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    set(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "${CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS} -undefined dynamic_lookup")
	message( "-- Set \"-undefined dynamic_lookup\" for shared libraries")
endif()

add_subdirectory(ccnx/forwarder/athena)
