#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QStyleFactory>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationName("MPlayer");
    QApplication::setOrganizationName("MPlayerProj");
    QApplication::setStyle(QStyleFactory::create("fusion"));

    const QString appIconPath = QDir(QCoreApplication::applicationDirPath()).filePath("assets/img/icon.ico");
    const QString sourceIconPath = QDir(QString::fromUtf8(PROJECT_ASSETS_DIR)).filePath("img/icon.ico");
    app.setWindowIcon(QIcon(QFileInfo::exists(appIconPath) ? appIconPath : sourceIconPath));

    MainWindow window;
    window.show();

    return app.exec();
}
