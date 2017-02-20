#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "canvas.h"

#include <QFileInfo>
#include <QPixmap>
#include <QScrollArea>
#include <QKeyEvent>

#include <iostream>

MainWindow::MainWindow(QDir const &path, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    path(path)
{
    ui->setupUi(this);

    state = std::shared_ptr <State>(new State());
    canvas = new Canvas(state);

    connect(ui->scaleSlider, &QSlider::valueChanged, canvas, &Canvas::zoom);

    connect(ui->brushWidth, &QSlider::valueChanged, canvas, &Canvas::setBrushWidth);

    connect(ui->nextImage, &QPushButton::clicked, this, &MainWindow::nextImage);


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

    canvas->setLabel(1);

    ui->scrollArea->setWidget(canvas);
    nextImage();
}

void MainWindow::keyPressEvent(QKeyEvent *event) {

   size_t number = event->key() - '0';
   if(number >= 1 && number <= config->labels.size()) {
      int label = number;
      ui->labelList->setCurrentRow(label - 1);

      canvas->setLabel(label);
   }


   if(event->key() == '-') {
       int value = ui->scaleSlider->value();
       ui->scaleSlider->setValue(value - ui->scaleSlider->pageStep());
   }

   if(event->key() == '+') {
       int value = ui->scaleSlider->value();
       ui->scaleSlider->setValue(value + ui->scaleSlider->pageStep());
   }
   if(event->key() == '[') {
       float value = ui->brushWidth->value();
       ui->brushWidth->setValue(value - std::max<int>(1, value * 0.1));
   }

   if(event->key() == ']') {
       float value = ui->brushWidth->value();
       ui->brushWidth->setValue(value + std::max<int>(1, value * 0.1));
   }

}



bool MainWindow::loadImage(QString const &path) {

    std::cout << "loading: " << path.toStdString() << std::endl;

    QPixmap p;
    if(p.load(path)) {
        state->areas.clear();
        state->filename = path;

        setWindowTitle(path);
        canvas->setImage(p);

        return true;
    }

    return false;
}



void MainWindow::nextImage() {

    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.PNG" << "*.jpeg" << "*.JPG" << "*.JPEG";


    QFileInfoList entries = path.entryInfoList(filters, QDir::Files|QDir::NoDotAndDotDot);
    int i = 0;

    if(currentEntry) {
        i = entries.indexOf(*currentEntry) + 1;
    }

    for(; i < entries.size(); ++i) {
        QFileInfo const &e = entries[i];

        QString name = e.filePath();
        QFileInfo annot (e.path() + "/" + e.completeBaseName() + ".json");

        if(ui->freshImage->isChecked() && annot.exists())
            continue;

        QPixmap p;
        if(loadImage(name)) {
            currentEntry = e;
            return;
        }
    }

    currentEntry.reset();
}


MainWindow::~MainWindow()
{
    delete ui;
}
