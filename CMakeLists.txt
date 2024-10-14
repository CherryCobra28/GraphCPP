cmake_minimum_required(VERSION 3.18)
project(Proj)

find_package(igraph REQUIRED)

add_executable(proj proj.cpp)
target_link_libraries(proj PUBLIC igraph::igraph)
