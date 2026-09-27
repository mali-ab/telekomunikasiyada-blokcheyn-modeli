#include "main_window.h"
#include "ui_helpers.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>

namespace {
void locateEnvFile() {
    if (QFile::exists(".env")) return;
    QDir appDir(QCoreApplication::applicationDirPath());
    if (appDir.exists(".env")) {
        QDir::setCurrent(appDir.absolutePath());
    } else if (appDir.exists("../.env")) {
        QDir::setCurrent(appDir.filePath(".."));
    }
}
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyleSheet(applicationStyle());
    locateEnvFile();
    MainWindow window;
    window.show();
    return app.exec();
}
