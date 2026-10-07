#include "mainwindow.h"

#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("InertialNavigation"));
    QApplication::setStyle(QStringLiteral("Fusion"));
    QFile theme(QStringLiteral(":/style.qss"));
    if (theme.open(QIODevice::ReadOnly))
        app.setStyleSheet(QString::fromUtf8(theme.readAll()));

    MainWindow window;
    window.show();

    return app.exec();
}
