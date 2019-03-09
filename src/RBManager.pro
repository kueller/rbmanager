#-------------------------------------------------
#
# Project created by QtCreator 2016-12-20T18:41:09
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = rbmanager
TEMPLATE = app


SOURCES += main.cpp\
        rbmanager.cpp \
    usbmanager.cpp \
    usbdisplay.cpp \
    size.c \
    confile.cpp \
    confileinfo.cpp

HEADERS  += rbmanager.h \
    usbmanager.h \
    usbdisplay.h \
    size.h \
    confile.h \
    confileinfo.h

FORMS    += rbmanager.ui \
    usbdisplay.ui \
    confileinfo.ui

CONFIG += console \
    c++11
