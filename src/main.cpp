#include <QApplication>
#include <QStyleFactory>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationName("MPlayer");
    QApplication::setOrganizationName("MPlayerProj");
    QApplication::setStyle(QStyleFactory::create("fusion"));

    MainWindow window;
    window.show();

    return app.exec();
}
