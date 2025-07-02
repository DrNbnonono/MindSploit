QT -= gui widgets
QT += core network sql

CONFIG += c++17 console
CONFIG -= app_bundle

TARGET = MindSploit
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/core/terminal_interface.cpp \
    src/core/engine_manager.cpp \
    src/core/session_manager.cpp \
    src/core/command_parser.cpp \
    src/engines/network/network_engine.cpp \
    src/ai/ai_manager.cpp \
    src/utils/network_utils.cpp \
    src/core/database.cpp \
    src/core/config_manager.cpp

HEADERS += \
    src/core/terminal_interface.h \
    src/core/engine_manager.h \
    src/core/session_manager.h \
    src/core/command_parser.h \
    src/engines/engine_interface.h \
    src/engines/network/network_engine.h \
    src/ai/ai_manager.h \
    src/utils/network_utils.h \
    src/core/database.h \
    src/core/config_manager.h

INCLUDEPATH += src

# Windows特定库
win32 {
    LIBS += -lws2_32 -liphlpapi
}
