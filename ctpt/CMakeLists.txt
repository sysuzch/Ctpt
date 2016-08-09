

project(ctpt CXX C)

cmake_minimum_required(VERSION 3.1)

find_library(gtestlib gtest)

option(WIN32_EN "select" OFF)
option(LINUX_EN "select" ON)

if (WIN32_EN)
  add_subdirectory(win32)
else()
  if(NOT CMAKE_CROSSCOMPILING)
    find_library(linuxfftwlib fftw3)
    set(lib_dir linux)
  else()
    unset(gtestlib CACHE )
    message("using fftw3 interface")
    add_library(fftw3f INTERFACE )
    target_link_libraries(fftw3f INTERFACE ${CMAKE_CURRENT_LIST_DIR}/armhf/libfftw3f.a )
    target_include_directories(fftw3f INTERFACE armhf)
    add_library(fftw3 INTERFACE )
    target_link_libraries(fftw3 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/armhf/libfftw3.a )
    target_include_directories(fftw3 INTERFACE armhf)
#    install(FILES armhf/fftw3.h  DESTINATION include )
#   install(FILES armhf/libfftw3f.a DESTINATION lib/armhf )
    set(lib_dir armhflinux)
  endif()  
endif()


set(sources
  calculator.cxx
  raw_wave.cxx
  calculator.hpp
  field_excitation_calculator.hpp
  raw_wave.hpp
  exciting_scanner.cxx
  exciting_scanner.hpp
  direct_scanner.cxx
  direct_scanner.hpp
  ctpt_api.cxx data_source.cxx
  fpga.cxx
  data_source_fpga.cxx
  )

set(simulation_source
  test/main.cxx
  test/test.cxx
  test/test2.cxx
  test/test_fpga.cxx
  test/test_raw.cxx

  )

#field_excitation_calculator.cxx

#add_library( ctpt SHARED ${sources} smc_api.c )
add_library( ctpt SHARED ${sources} smc_api.c )
add_library( ctpt_sim SHARED ${sources}   wave_gen.cxx
    wave-signal.c  wave-signal.h
)
target_include_directories(ctpt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories(ctpt_sim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

if(LINUX_EN)
  target_compile_options(ctpt PUBLIC -Wall -DCPTAPI_DLL -g -O0)
  target_compile_options(ctpt_sim PUBLIC -Wall -DCPTAPI_DLL -g -O0 -DNO_LOG)
endif()

target_link_libraries(ctpt PUBLIC fftw3 -lpthread)
target_link_libraries(ctpt_sim PUBLIC fftw3 -lpthread )

if(WIN32_EN)
#  target_link_libraries(ctpt PUBLIC fftw3)
  target_compile_options(ctpt PUBLIC -Wall -DWIN32 -DCPTAPI_DLL -g -O0)
  target_compile_options(ctpt_sim PUBLIC -Wall -DWIN32 -DCPTAPI_DLL -g -O0)
  set(CMAKE_SHARED_LINKER_FLAGS -Wl,--out-implib,libctpt.a)
  set(CMAKE_SHARED_LINKER_FLAGS_DEBUG -Wl,--out-implib,libctpt.a)
  set(lib_dir win32-gcc)
endif()


install(TARGETS ctpt
  ARCHIVE DESTINATION lib/${lib_dir}
  LIBRARY DESTINATION lib/${lib_dir}
  RUNTIME DESTINATION bin/${lib_dir}
  )

install(FILES ctpt_api.hpp DESTINATION include )


add_executable(demo1 test/demo.cxx)
target_link_libraries(demo1 PUBLIC ctpt)

if(gtestlib)
  target_link_libraries(ctpt_sim PUBLIC ${gtestlib})
  add_executable(rungtester ${simulation_source} )
  target_compile_options(rungtester PUBLIC -g -O0)
  target_link_libraries(rungtester PUBLIC ctpt_sim)
endif()

add_executable(fpga test/demo_fpga.cxx)

target_link_libraries(fpga PUBLIC ctpt)
