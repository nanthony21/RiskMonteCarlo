cmake_minimum_required(VERSION 3.15)
project(risk_sim LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(extern/pybind11)

add_library(
    rslib
    cpp_src/solver.cpp
    cpp_src/solver.hpp
    )

find_package(TBB CONFIG)
if (TBB_Found)
    target_link_libraries(rslib TBB::tbb)
else ()
    target_compile_definitions(rslib PRIVATE "RISK_SINGLE_THREAD")
endif ()

option(RiskSim_BUILD_TESTS OFF "Build tests")
if (RiskSim_BUILD_TESTS)
    add_executable(test tests/main.cpp)
    target_link_libraries(test PRIVATE rslib)
endif()

# The target name must match the name used in your C++ PYBIND11_MODULE macro
pybind11_add_module(risk_solver cpp_src/module.cpp)
target_link_libraries(risk_solver PRIVATE rslib)
install(TARGETS risk_solver DESTINATION .)