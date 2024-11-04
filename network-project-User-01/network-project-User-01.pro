QT += core gui qml quick multimedia

CONFIG += c++17

PATH_TO_LIBDATACHANNEL = D:/qtproject/libdatachannel
PATH_TO_OPUS = D:/qtproject/opus
#PATH_TO_SIO = D:/qtproject/socket.io-client-cpp
PATH_TO_BOOST = D:/qtproject/boost_1_86_0
PATH_TO_OPENSSL = "C:/Program Files/OpenSSL-Win64"
PATH_TO_ASIO = D:/qtproject/asio-1.30.2

SOURCES += \
    audioinput.cpp \
    audiooutput.cpp \
    main.cpp \
    signalingclient.cpp \
    webrtc.cpp \
#    $$PWD/SocketIO/sio_client.cpp \
#   $$PWD/SocketIO/sio_socket.cpp \
#    $$PWD/SocketIO/internal/sio_client_impl.cpp \
#    $$PWD/SocketIO/internal/sio_packet.cpp

HEADERS += \
    audioinput.h \
    audiooutput.h \
    signalingclient.h \
    webrtc.h \
#    $$PWD/SocketIO/sio_client.h \
#    $$PWD/SocketIO/sio_message.h \
#    $$PWD/SocketIO/sio_socket.h \
#    $$PWD/SocketIO/internal/sio_client_impl.h \
#    $$PWD/SocketIO/internal/sio_packet.h

DISTFILES += \
    main.qml


INCLUDEPATH += $$PATH_TO_LIBDATACHANNEL/include
LIBS += -L$$PATH_TO_LIBDATACHANNEL/Windows/Mingw64 -ldatachannel


INCLUDEPATH += $$PATH_TO_OPENSSL/include
LIBS += -L$$PATH_TO_OPENSSL/lib/VC/x64/MT -lssl -lcrypto


INCLUDEPATH += $$PATH_TO_OPUS/include
LIBS += -L$$PATH_TO_OPUS/build -lopus


LIBS += -lws2_32
LIBS += -lssp


LIBS += -L$$PATH_TO_BOOST/stage/lib
INCLUDEPATH += $$PATH_TO_BOOST


INCLUDEPATH += $$PATH_TO_ASIO/include


INCLUDEPATH += $$PATH_TO_SIO/lib/websocketpp
INCLUDEPATH += $$PATH_TO_SIO/lib/rapidjson/include


DEFINES += _WEBSOCKETPP_CPP11_STL_
DEFINES += _WEBSOCKETPP_CPP11_FUNCTIONAL_
DEFINES += SIO_TLS
DEFINES += ASIO_STANDALONE
DEFINES += BOOST_ASIO_HAS_STD_ADDRESSOF
DEFINES += BOOST_ASIO_USE_BOOST_REGEX
DEFINES += BOOST_ASIO_SEPARATE_COMPILATION
DEFINES += BOOST_ASIO_HAS_STD_CHRONO
DEFINES += BOOST_ASIO_ENABLE_HANDLER_TRACKING


RESOURCES += \
    resources.qrc


QMAKE_CXXFLAGS += -Wno-deprecated-declarations -Wno-unused-parameter -Wno-deprecated-copy -Wno-class-memaccess


QT += websockets

