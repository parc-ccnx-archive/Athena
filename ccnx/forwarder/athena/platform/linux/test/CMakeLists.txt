# Enable gcov output for the tests
add_definitions(--coverage)
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} " --coverage")

if( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
  set(TestsExpectedToPass
    test_athena_Ethernet
  )
endif()
  
foreach(test ${TestsExpectedToPass})
   AddTest(${test})
endforeach()

