cmake_minimum_required (VERSION 3.8)
project(gem5_wrapper C CXX)

aux_source_directory(./src GEM5_TLM_SRC)

if(NOT EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    message(FATAL_ERROR "The file conanbuildinfo.cmake doesn't exist, you have to run conan install first, or setup Conan plugin if using CLion")
endif()

set(CONAN_CMAKE_SILENT_OUTPUT TRUE)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)

conan_basic_setup(NO_OUTPUT_DIRS TARGETS)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_BINARY_DIR} $(CONAN_CMAKE_MODULE_PATH))
find_package(SystemCLanguage REQUIRED)
find_package(gem5 REQUIRED)

#set(GEM5_HOME /home/pzy/Documents/gem5/gem5/)
#link_directories(${GEM5_HOME}/build/RISCV/libgem5_opt.so)

#include_directories( "/home/pzy/Downloads/systemc-2.3.3/include")
#link_directories("/home/pzy/Downloads/systemc-2.3.3/lib-linux64")

add_library(gem5_wrapper STATIC
    ${GEM5_TLM_SRC}
)

target_compile_features(gem5_wrapper PRIVATE cxx_std_20)

target_include_directories(gem5_wrapper PRIVATE 
    "${PROJECT_SOURCE_DIR}/include/"
    #"${PROJECT_SOURCE_DIR}/include/ip/tlm_chi"
    "${CONAN_INCLUDE_DIRS}"
    ${CONAN_INCLUDE_DIRS_SYSTEMC}
    ${CONAN_GEM5_ROOT}/RISCV
    #${GEM5_HOME}/build/RISCV
    )

#target_link_libraries(gem5_wrapper PUBLIC ext_ip)
target_link_libraries(gem5_wrapper PUBLIC SystemC::systemc gem5::gem5)
target_compile_options(gem5_wrapper PUBLIC -fPIC -DTRACING_ON)
