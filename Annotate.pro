#-------------------------------------------------
#
# Project created by QtCreator 2017-02-08T17:07:41
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Annotate
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    canvas.cpp \
    state.cpp

HEADERS  += mainwindow.h \
    canvas.h \
    state.h

FORMS    += mainwindow.ui

CONFIG += c++11
QMAKE_CXXFLAGS += --std=c++11 `pkg-config opencv --cflags`
LIBS = `pkg-config --libs opencv`
