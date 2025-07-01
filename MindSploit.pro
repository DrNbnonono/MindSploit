QT += widgets

CONFIG += c++11

TARGET = MindSploit
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/gui/mainwindow.cpp \
    src/core/command.cpp \
    src/ai/ai_manager.cpp \
    src/modules/nmap_module.cpp \
    src/utils/process.cpp

HEADERS += \
    src/gui/mainwindow.h \
    src/core/command.h \
    src/ai/ai_manager.h \
    src/modules/nmap_module.h \
    src/utils/process.h

INCLUDEPATH += src
