QT += core gui widgets network

CONFIG += c++14
CONFIG -= app_bundle
TEMPLATE = app
TARGET = WinTools

DEFINES += QT_DEPRECATED_WARNINGS NOMINMAX _WIN32_WINNT=0x0601 WINVER=0x0601

QMAKE_CXXFLAGS += -Wno-missing-field-initializers

INCLUDEPATH += src

SOURCES += \
    src/main.cpp \
    src/domain/item.cpp \
    src/domain/config.cpp \
    src/domain/config_store.cpp \
    src/domain/path_resolver.cpp \
    src/domain/command_builder.cpp \
    src/domain/browser_catalog.cpp \
    src/domain/launch_planner.cpp \
    src/domain/launch_runner.cpp \
    src/domain/logger.cpp \
    src/platform/win/win_utils.cpp \
    src/platform/win/system_environment.cpp \
    src/platform/win/shell_launcher.cpp \
    src/platform/win/process_enumerator.cpp \
    src/platform/win/elevation.cpp \
    src/platform/win/autostart_manager.cpp \
    src/platform/win/icon_factory.cpp \
    src/platform/win/registry_reader.cpp \
    src/platform/win/single_instance.cpp \
    src/ui/mode_dialog.cpp \
    src/ui/item_dialog.cpp \
    src/ui/settings_dialog.cpp \
    src/ui/launch_result_dialog.cpp \
    src/ui/tray_controller.cpp \
    src/ui/drag_widgets.cpp \
    src/ui/main_window.cpp

HEADERS += \
    src/domain/interfaces.h \
    src/domain/item.h \
    src/domain/config.h \
    src/domain/config_store.h \
    src/domain/path_resolver.h \
    src/domain/command_builder.h \
    src/domain/browser_catalog.h \
    src/domain/launch_planner.h \
    src/domain/launch_runner.h \
    src/domain/logger.h \
    src/platform/win/win_utils.h \
    src/platform/win/system_environment.h \
    src/platform/win/shell_launcher.h \
    src/platform/win/process_enumerator.h \
    src/platform/win/elevation.h \
    src/platform/win/autostart_manager.h \
    src/platform/win/icon_factory.h \
    src/platform/win/registry_reader.h \
    src/platform/win/single_instance.h \
    src/ui/mode_dialog.h \
    src/ui/item_dialog.h \
    src/ui/settings_dialog.h \
    src/ui/launch_result_dialog.h \
    src/ui/tray_controller.h \
    src/ui/drag_widgets.h \
    src/ui/main_window.h

RESOURCES += src/resources/resources.qrc

win32: LIBS += -lshell32 -luserenv -ladvapi32
