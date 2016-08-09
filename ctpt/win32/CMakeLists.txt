

add_library(fftw3 INTERFACE)
target_include_directories(fftw3 INTERFACE .)
target_link_libraries(fftw3 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/libfftw3-3.a)


install(FILES libfftw3-3.a libfftw3-3.dll DESTINATION lib/win32-gcc )




