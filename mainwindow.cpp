#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "canvas.h"

#include <QFileInfo>
#include <QPixmap>

#include <iostream>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    state = std::shared_ptr<State>(new State());
    canvas = new Canvas(state);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(canvas);

    ui->holder->setLayout(layout);
}


bool MainWindow::loadImage(QString const &path) {

    std::cout << "loading: " << path.toStdString() << std::endl;

    QPixmap p;
    if(p.load(path)) {
        state->areas.clear();
        state->currentLabel = 0;
        state->filename = path;

        setWindowTitle(path);
        canvas->setImage(p);

        return true;
    }

    return false;
}


bool MainWindow::nextImage(QDir const &path) {

    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.PNG" << "*.jpeg" << "*.JPG" << "*.JPEG";


    QFileInfoList entries = path.entryInfoList(filters, QDir::Files|QDir::NoDotAndDotDot);

    for(QFileInfo const &e : entries) {

        QString name = e.filePath();
        QFileInfo annot (e.path() + "/" + e.completeBaseName() + ".json");

        if(annot.exists())
            continue;

        QPixmap p;
        if(loadImage(name))
            return true;
    }

    return false;

}


MainWindow::~MainWindow()
{
    delete ui;
}
