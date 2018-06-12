#-------------------------------------------------
#
# Project created by QtCreator 2018-04-11T10:43:30
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = FileTransferServer
TEMPLATE = app

win32 {
GIT_VERSION = $$system(git -C $$_PRO_FILE_PWD_ describe)
} else {
GIT_VERSION = "$(shell git -C \""$$_PRO_FILE_PWD_"\" describe)"
}
DEFINES += GIT_VERSION=\\\"$$GIT_VERSION\\\"


SOURCES += main.cpp\
        server.cpp

HEADERS  += server.h

FORMS    += server.ui
