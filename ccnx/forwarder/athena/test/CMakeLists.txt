# Enable gcov output for the tests
add_definitions(--coverage)
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} " --coverage")

set(TestsExpectedToPass
    test_athena
    test_athena_FIB
    test_athena_PIT
    test_athena_TransportLinkAdapter
    test_athena_TransportLink
    test_athena_TransportLinkModule
    test_athena_TransportLinkModuleTCP
    test_athena_TransportLinkModuleUDP
    test_athena_TransportLinkModuleETH
    test_athena_TransportLinkModuleTEMPLATE
    test_athena_ContentStore
    test_athena_LRUContentStore
    test_athena_InterestControl
    test_athenactl
)

foreach(test ${TestsExpectedToPass})
    AddTest(${test})
endforeach()
