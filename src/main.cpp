#include <QApplication>

#include "MainWindow.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.setWindowTitle("Velix Installer");
    mainWindow.resize(1120, 720);
    mainWindow.show();

    return app.exec();
}
