#-------------------------------------------------
#
# Project created by QtCreator 2016-04-18T15:07:39
#
#-------------------------------------------------

QT       += core gui network widgets winextras multimedia
CONFIG -= debug_and_release debug_and_release_target

CONFIG(release, debug|release){
    TARGET = oculus-sdk
    DESTDIR = ./build/release
}

CONFIG(debug, debug|release){
    TARGET = oculus-sdk-debug
    DESTDIR = ./build/debug
}

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.rcc
UI_DIR = $$DESTDIR/.ui

win32 {
    QMAKE_CXXFLAGS += /std:c++20
}
unix {
    QMAKE_CXXFLAGS += -fvisibility=hidden

    # Use this for GCC 8 or 9
    # QMAKE_CXXFLAGS += -std=c++2a
    # Use this for GCC > 9
    QMAKE_CXXFLAGS += -std=c++20
}

SOURCES += main.cpp\
    Oculus/OsClientCtrl.cpp \
    Oculus/OsStatusRx.cpp \
    RmUtil/RmUtil.cpp \
    RmGl/RmGlOrtho.cpp \
    RmGl/RmGlSurface.cpp \
    RmGl/RmGlWidget.cpp \
    Displays/SonarSurface.cpp \
    OculusSonar/MainView.cpp \
    OculusSonar/OnlineCtrls.cpp \
    OculusSonar/EnvCtrls.cpp \
    OculusSonar/ReviewCtrls.cpp \
    OculusSonar/SettingsCtrls.cpp \
    OculusSonar/CtrlWidget.cpp \
    RmGl/PalWidget.cpp \
    RmUtil/RmLogger.cpp \
    RmUtil/RmPlayer.cpp \
    OculusSonar/SettingsForm.cpp \
    OculusSonar/ConnectForm.cpp \
    OculusSonar/ToolsCtrls.cpp \
    OculusSonar/ModeCtrls.cpp \
    OculusSonar/OptionsCtrls.cpp \
    OculusSonar/DeviceForm.cpp \
    OculusSonar/TitleCtrls.cpp \
    OculusSonar/AppCtrls.cpp \
    OculusSonar/InfoCtrls.cpp \
    OculusSonar/CursorCtrls.cpp \
    OculusSonar/InfoForm.cpp \
    Controls/RangeSlider.cpp \
    OculusSonar/HelpForm.cpp

HEADERS  += \
    Oculus/Oculus.h \
    Oculus/OsClientCtrl.h \
    Oculus/OsStatusRx.h \
    RmUtil/RmUtil.h \
    RmGl/RmGlOrtho.h \
    RmGl/RmGlSurface.h \
    RmGl/RmGlWidget.h \
    Displays/SonarSurface.h \
    OculusSonar/MainView.h \
    OnlineCtrls.h \
    OculusSonar/EnvCtrls.h \
    OculusSonar/ReviewCtrls.h \
    OculusSonar/SettingsCtrls.h \
    OculusSonar/CtrlWidget.h \
    RmGl/PalWidget.h \
    RmUtil/RmLogger.h \
    RmUtil/RmPlayer.h \
    OculusSonar/SettingsForm.h \
    OculusSonar/ConnectForm.h \
    Oculus/OssDataWrapper.h \
    Oculus/DataWrapper.h \
    OculusSonar/ToolsCtrls.h \
    OculusSonar/ModeCtrls.h \
    OculusSonar/OptionsCtrls.h \
    OculusSonar/DeviceForm.h \
    OculusSonar/TitleCtrls.h \
    OculusSonar/AppCtrls.h \
    OculusSonar/InfoCtrls.h \
    OculusSonar/CursorCtrls.h \
    OculusSonar/OnlineCtrls.h \
    OculusSonar/InfoForm.h \
    Controls/RangeSlider.h \
    Controls/RangeSlider_p.h \
    OculusSonar/HelpForm.h

FORMS    += \
    OculusSonar/OnlineCtrls.ui \
    OculusSonar/EnvCtrls.ui \
    OculusSonar/ReviewCtrls.ui \
    OculusSonar/SettingsCtrls.ui \
    OculusSonar/SettingsForm.ui \
    OculusSonar/ConnectForm.ui \
    OculusSonar/ToolsCtrls.ui \
    OculusSonar/ModeCtrls.ui \
    OculusSonar/OptionsCtrls.ui \
    OculusSonar/DeviceForm.ui \
    OculusSonar/TitleCtrls.ui \
    OculusSonar/AppCtrls.ui \
    OculusSonar/InfoCtrls.ui \
    OculusSonar/CursorCtrls.ui \
    OculusSonar/InfoForm.ui \
    OculusSonar/HelpForm.ui

CONFIG += c++11


RESOURCES += \
    Media/Media.qrc \
    OculusSonar/shaders.qrc

RC_FILE = Oculus.rc


INCLUDEPATH = \
    "OculusSonar/" \
    "C:/Program Files (x86)/Windows Kits/10/Include/10.0.10240.0/ucrt" \
    "C:/Program Files (x86)/Windows Kits/10/Include/10.0.10586.0/um"


# Will need to update this for dedicated x64 builds of the software
LIBS += \
    -L"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.10240.0/ucrt/x86" \
    -L"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.10586.0/um/x86" \
    -luser32 \
    -lgdi32 \
    -lole32
