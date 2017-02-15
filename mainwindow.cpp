#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "canvas.h"

#include <QFileInfo>
#include <QPixmap>
#include <QScrollArea>
#include <QKeyEvent>

#include <iostream>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    state = std::shared_ptr <State>(new State());
    canvas = new Canvas(state);

    connect(ui->scaleSlider, &QSlider::valueChanged, canvas, &Canvas::zoom);

    config = std::shared_ptr<Config>(new Config());
    config->labels = {"trunk", "above whirl"};

    int label = 1;
    for(std::string const& l : config->labels) {
        QListWidgetItem *item = new QListWidgetItem(l.c_str());
        item->setData(Qt::UserRole, QVariant(label++));
        ui->labelList->addItem(item);
    }

    ui->labelList->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->labelList->setCurrentRow(0);

    //QListWidgetItem *item = ui->labelList->currentItem();
    //std::cout << item->data(Qt::UserRole).toInt() << std::endl;

    ui->scrollArea->setWidget(canvas);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {

   int number = event->key() - '0';
   if(number >= 1 && number <= config->labels.size()) {
      int label = number - 1;
      ui->labelList->setCurrentRow(label);

      canvas->setLabel(label);
   }
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
