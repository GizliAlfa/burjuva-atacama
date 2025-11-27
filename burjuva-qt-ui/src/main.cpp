#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application info
    app.setApplicationName("Burjuva-Atacama Control Panel");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Burjuva");
    
    // Set Fusion style for better cross-platform look
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Create and show main window
    MainWindow window;
    window.show();
    
    qDebug() << "Burjuva-Atacama Control Panel started";
    
    return app.exec();
}
