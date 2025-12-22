##
# CMakeLists.txt for QPitch - Program for tuning instruments
#
# Copyright (c) 2008-2009 Nico Schlömer <nico.schloemer@gmx.net>
#                         William Spinelli <wylliam@tiscali.it>
##

# set object files dependencies for the executable
add_executable( qpitch
    main.cpp
    qaboutdlg.cpp
    qlogview.cpp
    qpitch.cpp
    qpitchcore.cpp
    qsettingsdlg.cpp
    fpsprofiler.cpp
    visualization_data.cpp
    freqdiffview.cpp
    notes.cpp
    cyclicbuffer.cpp
    qpitchsettings.cpp
    pitchdetection.cpp
    texthelper.cpp
    plotview.cpp

    qaboutdlg.h
    qlogview.h
    qpitchcore.h
    qpitch.h
    qsettingsdlg.h
    fpsprofiler.h
    visualization_data.h
    freqdiffview.h
    notes.h
    cyclicbuffer.h
    qpitchsettings.h
    pitchdetection.h
    texthelper.h
    plotview.h

    ui/qpitch.qrc

    ui/qaboutdlg.ui
    ui/qpitch.ui
    ui/qsettingsdlg.ui
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# add library dependencies needed by the executable (variables are filled
# by FIND_PACKAGE)
target_link_libraries( qpitch
    Qt::Widgets
    PkgConfig::portaudio-2.0
    ${FFTW3_LIBRARIES}
)


# where to install files
install(
    TARGETS qpitch
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/ui/icons/qpitch.xpm
    DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps/
)

install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/qpitch.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications/
)

qt_add_executable(cyclicbuffertest
    tst_cyclicbuffertest.cpp
    tst_cyclicbuffertest.h
    cyclicbuffer.cpp
    cyclicbuffer.h
)

add_test(NAME cyclicbuffertest COMMAND cyclicbuffertest)
target_link_libraries(cyclicbuffertest PRIVATE Qt::Test)
