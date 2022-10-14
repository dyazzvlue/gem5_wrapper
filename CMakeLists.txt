aux_source_directory(./src GEM5_TLM)

set(GEM5_HOME /home/pzy/Documents/gem5/gem5/)
link_directories(${GEM5_HOME}/build/RISCV/libgem5_opt.so)

include_directories( "/home/pzy/Downloads/systemc-2.3.3/include")
link_directories("/home/pzy/Downloads/systemc-2.3.3/lib-linux64")

add_library(gem5_tlm STATIC
    ${GEM5_TLM}
)

target_compile_features(gem5_tlm PRIVATE cxx_std_20)

target_include_directories(gem5_tlm PRIVATE 
    "${PROJECT_SOURCE_DIR}/include/"
    #"${PROJECT_SOURCE_DIR}/include/ip/tlm_chi"
    ${CONAN_INCLUDE_DIRS_SYSTEMC}
    ${GEM5_HOME}/build/RISCV
    )

#target_link_libraries(gem5 PUBLIC ext_ip)
target_link_libraries(gem5_tlm PUBLIC systemc)
target_compile_options(gem5_tlm PUBLIC -fPIC -DTRACING_ON)
