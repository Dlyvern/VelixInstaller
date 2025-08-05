#include <QApplication>

#include "MainWindow.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.setWindowTitle("Velix Installer");
    mainWindow.resize(800, 600);
    mainWindow.show();

    return app.exec();
}