#include "mainwindow.h"
#include <QApplication>

#include <QCommandLineParser>
#include <iostream>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("annotate");
    QApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("Test helper");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("source", QCoreApplication::translate("main", "Source directory to scan."));


    parser.process(app);

    const QStringList args = parser.positionalArguments();\

    QDir path;
    if(args.size() >= 1) {
        path = QDir(args.at(0));
    }

    if( !path.exists()) {
        std::cerr << "Directory does not exist: " << path.canonicalPath().toStdString() << std::endl;
        parser.showHelp(1);      \
    }



    std::cout << "Annotating images in path: " << path.canonicalPath().toStdString()  << std::endl;

    MainWindow w(path);
    if(w.hasImage()) {
        w.setWindowState(Qt::WindowMaximized);
        w.show();
        return app.exec();
    } else {
        std::cerr << "No (more) images found to annotate" << std::endl;
    }
}
