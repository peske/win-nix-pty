cmake_minimum_required(VERSION 3.15)
project(PtyNative)

set(CMAKE_CXX_STANDARD 17)

add_definitions(-DFROM_CLION_CMAKE)

add_executable(PtyNative main.cpp logging.cpp logging.h command_processor.cpp command_processor.h io_processor.cpp io_processor.h file_helpers.cpp file_helpers.h helpers.cpp helpers.h stand_alone_io.cpp stand_alone_io.h includes.h)
