cmake_minimum_required(VERSION 3.15)
project(risk_sim LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(extern/pybind11)
#find_package(pybind11 CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)

add_library(tester solver.cpp
        solver.hpp)
target_link_libraries(tester TBB::tbb)

add_executable(test main.cpp)
target_link_libraries(test PUBLIC tester)

# The target name must match the name used in your C++ PYBIND11_MODULE macro
pybind11_add_module(risk_solver_ext module.cpp)
target_link_libraries(risk_solver_ext PUBLIC tester)
install(TARGETS risk_solver_ext DESTINATION .)