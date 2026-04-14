add_executable(analyzer_tests
    test_implementations.cpp
    test_configParser.cpp
    test_solverFactory.cpp
    test_streamWriter.cpp
)

target_link_libraries(analyzer_tests PRIVATE analyzer_lib Catch2::Catch2WithMain)

catch_discover_tests(analyzer_tests)

# When built with MPI, also run the test binary under mpirun -n 2 so the actual
# gather/gatherv path in MpiCPU is exercised. CTest runs this separately from the
# single-process suite above.
if(USE_MPI)
    find_program(MPIRUN mpirun)
    if(MPIRUN)
        add_test(NAME mpiCPU_gather_2ranks
            COMMAND ${MPIRUN} -n 2 $<TARGET_FILE:analyzer_tests> [mpi])
    endif()
endif()
