cmake_minimum_required(VERSION 3.15)
project(risk_sim LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

add_subdirectory(extern/pybind11)
#find_package(pybind11 CONFIG REQUIRED)

add_library(tester solver.cpp)
target_link_libraries(tester pybind11::pybind11)

# The target name must match the name used in your C++ PYBIND11_MODULE macro
pybind11_add_module(risk_solver_ext solver.cpp)

install(TARGETS risk_solver_ext DESTINATION .)