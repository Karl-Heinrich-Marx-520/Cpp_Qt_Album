QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    albumproject.cpp \
    albumwizard.cpp \
    confirmpage.cpp \
    main.cpp \
    mainwindow.cpp \
    picshow.cpp \
    prosetpage.cpp \
    slideshowwidget.cpp

HEADERS += \
    albumproject.h \
    albumwizard.h \
    confirmpage.h \
    mainwindow.h \
    picshow.h \
    prosetpage.h \
    slideshowwidget.h

FORMS += \
    confirmpage.ui \
    mainwindow.ui \
    prosetpage.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    rc.qrc
