#-------------------------------------------------
#
# Project created by QtCreator 2017-10-19T18:37:57
#
#-------------------------------------------------

QT       += core gui network testlib

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = desktopclient
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    telnetconnection.cpp \
    mapping_hirom.c \
    mapping_lorom.c \
    rommapping.c \
    inputdecoder.cpp

HEADERS  += mainwindow.h \
    telnetconnection.h \
    rommapping.h \
    inputdecoder.h

FORMS    += mainwindow.ui

RESOURCES += \
    images.qrc
