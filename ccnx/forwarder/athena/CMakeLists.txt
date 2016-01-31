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
    athena_FIB.c 
    athena_ContentStore.c 
    athena_LRUContentStore.c 
    athena_PIT.c 
    athena_TransportLinkAdapter.c 
    athena_TransportLink.c 
    athena_TransportLinkModule.c 
    athena_TransportLinkModuleTCP.c 
    athena_TransportLinkModuleUDP.c 
    athena_TransportLinkModuleETH.c 
	${ATHENA_PLATFORM_MODULES}
	)

add_library(athena         STATIC ${ATHENA_SOURCE_FILES})

set(athena_libraries
  athena
  )

foreach(lib ${athena_libraries})
  install(TARGETS ${lib} LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
  set_property(TARGET ${lib} PROPERTY C_STANDARD 99)
  set_property(TARGET ${lib} PROPERTY INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib")
endforeach()

add_subdirectory(test)
add_subdirectory(command-line)

if ( UNIX )
  execute_process(COMMAND whoami OUTPUT_VARIABLE USER 
	    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if ( ${USER} STREQUAL "root" )
	add_subdirector(platform/darwin/test)
	add_subdirectory(platform/linux/test)
  endif()
endif()
