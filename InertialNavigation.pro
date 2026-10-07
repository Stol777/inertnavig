QT += widgets

TEMPLATE = app
TARGET = InertialNavigation
CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    simulationmodel.cpp \
    navigationview.cpp \
    errorchart.cpp

HEADERS += \
    mainwindow.h \
    simulationmodel.h \
    navigationview.h \
    errorchart.h

FORMS += mainwindow.ui
RESOURCES += resources.qrc

qtHaveModule(multimedia) {
    QT += multimedia
    DEFINES += INS_WITH_SOUND
} else {
    message("Qt Multimedia not found: correction sound disabled")
}

msvc: QMAKE_CXXFLAGS += /utf-8
