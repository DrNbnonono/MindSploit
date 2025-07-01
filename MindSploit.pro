QT += widgets sql network

CONFIG += c++11

TARGET = MindSploit
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/gui/mainwindow.cpp \
    src/gui/terminal_widget.cpp \
    src/core/command.cpp \
    src/core/module_interface.cpp \
    src/core/module_manager.cpp \
    src/ai/ai_manager.cpp \
    src/modules/nmap/nmap_module.cpp \
    src/modules/searchsploit/searchsploit_module.cpp \
    src/modules/curl/curl_module.cpp \
    src/modules/portscan/portscan_module.cpp \
    src/utils/process.cpp \
    src/core/database.cpp \
    src/core/config_manager.cpp

HEADERS += \
    src/gui/mainwindow.h \
    src/gui/terminal_widget.h \
    src/core/command.h \
    src/core/module_interface.h \
    src/core/module_manager.h \
    src/ai/ai_manager.h \
    src/modules/nmap/nmap_module.h \
    src/modules/searchsploit/searchsploit_module.h \
    src/modules/curl/curl_module.h \
    src/modules/portscan/portscan_module.h \
    src/utils/process.h \
    src/core/database.h \
    src/core/config_manager.h

INCLUDEPATH += src
