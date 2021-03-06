#-------------------------------------------------
#
# Project created by QtCreator 2017-05-13T22:29:08
#
#-------------------------------------------------

QT       += core gui sql serialport printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qomanager
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

LRELEASE_DIR = I18n/
CONFIG += lrelease
CONFIG -= debug_and_release debug_and_release_target


SOURCES += main.cpp\
        mainwindow.cpp \
    classes.cpp \
    comboxeditor.cpp \
    comm.cpp \
    competitors.cpp \
    controlpoints.cpp \
    courses.cpp \
    database.cpp \
    defaultclasses.cpp \
    events.cpp \
    finish.cpp \
    gpxreader.cpp \
    iofreader.cpp \
    osqltable.cpp \
    parser.cpp \
    protocol.cpp \
    races.cpp \
    searchline.cpp \
    service.cpp \
    split_editor.cpp \
    uisettings.cpp \
    crc.c \
    spool.cpp

HEADERS  += mainwindow.h \
    classes.h \
    comboxeditor.h \
    comm.h \
    competitors.h \
    config.h \
    controlpoints.h \
    courses.h \
    crc.h \
    database.h \
    defaultclasses.h \
    disciplines.h \
    displaytime.h \
    events.h \
    fifo.h \
    finish.h \
    gpxreader.h \
    iofreader.h \
    osqltable.h \
    parser.h \
    protocol.h \
    races.h \
    ringbuffer.h \
    searchline.h \
    service.h \
    split_editor.h \
    uisettings.h \
    spool.h

RESOURCES += \
    resources.qrc

TRANSLATIONS = i18n/qomanager_ru.ts

#COPY_CONFIG = $$files(i18n/*.qm, true)
#copy_cmd.input = COPY_CONFIG
#copy_cmd.output = i18n/${QMAKE_FILE_IN_BASE}${QMAKE_FILE_EXT}
#copy_cmd.commands = $$QMAKE_COPY_FILE ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
#copy_cmd.CONFIG += no_link_no_clean
#copy_cmd.variable_out = PRE_TARGETDEPS
#QMAKE_EXTRA_COMPILERS += copy_cmd

win32:RC_ICONS += icons/micon.ico
win32:LIBS += -lwinspool
