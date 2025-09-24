project(FlySQ)

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
set(FLYSQ_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE FILEPATH "FlySQ source directory" FORCE)
set(FLYSQ_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}" CACHE FILEPATH "FlySQ binary directory" FORCE)

list(APPEND CMAKE_MODULE_PATH ${FLYSQ_SOURCE_DIR}/cmake)

include(flysq_list_make_absolute)
