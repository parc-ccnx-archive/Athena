set(ATHENA_SOURCE_FILES
    athena_main.c
    #../../athena_TransportLinkModuleTCP.c # uncomment this line to statically include the TCP link module
    #../../athena_TransportLinkModuleUDP.c # uncomment this line to statically include the UDP link module
    #../../athena_TransportLinkModuleETH.c # uncomment this line to statically include the ETH link module
    )

add_executable(athenabin ${ATHENA_SOURCE_FILES})
target_link_libraries(athenabin ${ATHENA_LINK_LIBRARIES})
set_target_properties(athenabin PROPERTIES OUTPUT_NAME athena)

install(TARGETS athenabin RUNTIME DESTINATION bin)
