#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "webrtc.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);


    qmlRegisterType<WebRTC>("com.example.network", 1, 0, "WebRTC");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
