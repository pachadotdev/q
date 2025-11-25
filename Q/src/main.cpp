#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setApplicationName("Q");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Q");
    app.setDesktopFileName("q");  // Without .desktop suffix
    
    // Suppress KDE file system watcher warnings
    qputenv("QT_LOGGING_RULES", "kf.kio.widgets.kdirmodel.debug=false;kf.jobwidgets.debug=false");
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
