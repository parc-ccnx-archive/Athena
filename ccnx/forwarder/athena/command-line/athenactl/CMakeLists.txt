add_executable(athenactl athenactl_main.c)
target_link_libraries(athenactl ${ATHENA_LINK_LIBRARIES})

install(TARGETS athenactl RUNTIME DESTINATION bin)
