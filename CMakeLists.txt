##
# CMakeLists.txt for QPitch - Program for tuning instruments
#
# Copyright (c) 2008-2009 Nico Schlömer <nico.schloemer@gmx.net>
#                         William Spinelli <wylliam@tiscali.it>
##

# set minimum cmake version
cmake_minimum_required( VERSION 4.1 )

project(qpitch)

set(CMAKE_CXX_STANDARD 23)

include(GNUInstallDirs)

# set path for additional cmake modules
set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_modules )
# find the required libraries and sets the corresponding environment variables
find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

find_package( Portaudio REQUIRED )
find_package( FFTW3 REQUIRED )

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

enable_testing(true)

# trigger the actual compilation of QPitch
add_subdirectory( src )
