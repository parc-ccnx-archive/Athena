# Define a few configuration variables that we want accessible in the software

configure_file(config.h.in config.h @ONLY)

if ( APPLE )
  set(ATHENA_PLATFORM_MODULES 
    platform/darwin/athena_Ethernet.c
    )
endif()

if( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
  set(ATHENA_PLATFORM_MODULES 
    platform/linux/athena_Ethernet.c
    )
endif()

set(ATHENA_SOURCE_FILES
    athena.c 
    athena_About.c 
    athenactl.c 
    athenactl_About.c 
    athena_Control.c 
    athena_InterestControl.c 
    athena_Fragmenter.c 
    athena_FIB.c 
    athena_ContentStore.c 
    athena_LRUContentStore.c 
    athena_PIT.c 
    athena_TransportLinkAdapter.c 
    athena_TransportLink.c 
    athena_TransportLinkModule.c 
    ${ATHENA_PLATFORM_MODULES}
    )

set(ATHENA_HEADER_FILES
    athena.h
    athena_About.h
    athena_ContentStore.h
    athena_ContentStoreInterface.h
    athena_Control.h
    athena_Ethernet.h
    athena_Fragmenter.h
    athena_FIB.h
    athena_InterestControl.h
    athena_LRUContentStore.h
    athena_PIT.h
    athena_TransportLink.h
    athena_TransportLinkAdapter.h
    athena_TransportLinkModule.h
    athenactl.h
    athenactl_About.h
    )

set(LIBATHENA_TCP_SOURCE_FILES
    athena_TransportLinkModuleTCP.c
    )

set(LIBATHENA_UDP_SOURCE_FILES
    athena_Fragmenter.c 
    athena_TransportLinkModuleUDP.c
    )

set(LIBATHENA_ETH_SOURCE_FILES
    athena_Fragmenter.c 
    athena_TransportLinkModuleETH.c
    ${ATHENA_PLATFORM_MODULES}
    )

set(LIBATHENA_FRAGMENT_BEFS_SOURCE_FILES
    athena_TransportLinkModuleFragmenter_BEFS.c
    )

set(LIBATHENA_TEMPLATE_SOURCE_FILES
    athena_TransportLinkModuleTEMPLATE.c
    )

add_library(athena_TCP.shared SHARED ${LIBATHENA_TCP_SOURCE_FILES})
set_target_properties(athena_TCP.shared PROPERTIES
  C_STANDARD 99
  SOVERSION 1
  VERSION 1.0
  OUTPUT_NAME athena_TCP )

add_library(athena_UDP.shared SHARED ${LIBATHENA_UDP_SOURCE_FILES})
set_target_properties(athena_UDP.shared PROPERTIES
  C_STANDARD 99
  SOVERSION 1
  VERSION 1.0
  OUTPUT_NAME athena_UDP )

add_library(athena STATIC ${ATHENA_SOURCE_FILES} ${ATHENA_HEADER_FILES})

source_group(Sources FILES ${ATHENA_HEADER_FILES})
source_group(Sources FILES ${ATHENA_SOURCE_FILES})
source_group(Sources FILES ${LIBATHENA_TCP_SOURCE_FILES})
source_group(Sources FILES ${LIBATHENA_UDP_SOURCE_FILES})
source_group(Sources FILES ${LIBATHENA_ETH_SOURCE_FILES})
source_group(Sources FILES ${LIBATHENA_FRAGMENT_BEFS_SOURCE_FILES})
source_group(Sources FILES ${LIBATHENA_TEMPLATE_SOURCE_FILES})

add_library(athena_ETH.shared SHARED ${LIBATHENA_ETH_SOURCE_FILES})
set_target_properties(athena_ETH.shared PROPERTIES
  C_STANDARD 99
  SOVERSION 1
  VERSION 1.0
  OUTPUT_NAME athena_ETH )

add_library(athena_Fragmenter_BEFS.shared SHARED ${LIBATHENA_FRAGMENT_BEFS_SOURCE_FILES})
set_target_properties(athena_Fragmenter_BEFS.shared PROPERTIES
  C_STANDARD 99
  SOVERSION 1
  VERSION 1.0
  OUTPUT_NAME athena_Fragmenter_BEFS )

add_library(athena_TEMPLATE.shared SHARED ${LIBATHENA_TEMPLATE_SOURCE_FILES})
set_target_properties(athena_TEMPLATE.shared PROPERTIES
  C_STANDARD 99
  SOVERSION 1
  VERSION 1.0
  OUTPUT_NAME athena_TEMPLATE )

set(athena_libraries
  athena
  athena_TCP.shared
  athena_UDP.shared
  athena_ETH.shared
  athena_Fragmenter_BEFS.shared
  athena_TEMPLATE.shared
  )

foreach(lib ${athena_libraries})
  install(TARGETS ${lib} LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
  set_property(TARGET ${lib} PROPERTY C_STANDARD 99)
endforeach()

add_subdirectory(test)
add_subdirectory(command-line)

if ( UNIX )
  execute_process(COMMAND whoami OUTPUT_VARIABLE USER 
        OUTPUT_STRIP_TRAILING_WHITESPACE)
  if ( ${USER} STREQUAL "root" )
    add_subdirectory(platform/darwin/test)
    add_subdirectory(platform/linux/test)
  endif()
endif()
