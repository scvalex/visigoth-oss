QT += core gui opengl xml network

RESOURCES = resources.qrc \

CONFIG += oauth
oauth {
    CONFIG += link_pkgconfig
    PKGCONFIG += qoauth
    DEFINES += HAS_OAUTH
}

TARGET = visigoth
TEMPLATE = app

SOURCES += main.cpp \
           mainwindow.cpp \
           node.cpp \
           edge.cpp \
           graphscene.cpp \
           glgraphwidget.cpp \
           glancillary.c \
           algorithm.cpp \
           barabasialbert.cpp \
           statistics.cpp \
           quadtree.cpp \
           erdosrenyi.cpp \
           wattsstrogatz.cpp \
           vtools.cpp \
           notify.cpp

HEADERS += mainwindow.h \
           node.h \
           edge.h \
           graphscene.h \
           glgraphwidget.h \
           glancillary.h \
           algorithm.h \
           statistics.h \
           quadtree.h \
           barabasialbert.h \
           erdosrenyi.h \
           wattsstrogatz.h \
           vtools.h \
           notify.h

FORMS += mainwindow.ui \
         erdoscontrol.ui \
         barabasialbert.ui \
         helpWidget.ui \
         statistics.ui \
         wattscontrol.ui

test {
    CONFIG -= release
    TARGET = test
    QT += testlib
    SOURCES -= main.cpp
    SOURCES += testsimple.cpp
    QMAKE_CXXFLAGS += -g -coverage -O0
    LIBS += -lgcov
}

oauth {
    SOURCES += twitter.cpp
    HEADERS += twitter.h
    FORMS += twitauthdialog.ui \
             twittercontrol.ui
}



OTHER_FILES += \
    helpFile.html
