add_executable(athenabin athena_main.c)
target_link_libraries(athenabin ${ATHENA_LINK_LIBRARIES})
set_target_properties(athenabin PROPERTIES OUTPUT_NAME athena)

install(TARGETS athenabin RUNTIME DESTINATION bin)
