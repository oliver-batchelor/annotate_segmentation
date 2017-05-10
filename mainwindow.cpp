#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "canvas.h"

#include <QFileInfo>
#include <QPixmap>
#include <QScrollArea>
#include <QKeyEvent>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QShortcut>
#include <QDebug>
#include <QActionGroup>
#include <QProcess>

#include <iostream>

#include <opencv2/imgcodecs.hpp>

MainWindow::MainWindow(QDir const &path, QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    path(path)
{

    ui->setupUi(this);

    canvas = new Canvas();\
    ui->scrollArea->setWidget(canvas);

    connect(ui->scaleSlider, &QSlider::valueChanged, canvas, &Canvas::zoom);
    connect(ui->nextImage, &QPushButton::clicked, this, &MainWindow::nextImage);

    connect(ui->prevImage, &QPushButton::clicked, this, &MainWindow::prevImage);

    connect(ui->discard, &QPushButton::clicked, this, &MainWindow::discardImage);


    connect(ui->action_Next, &QAction::triggered, this, &MainWindow::nextImage);
    connect(ui->action_Prev, &QAction::triggered, this, &MainWindow::prevImage);


    connect(ui->action_Undo, &QAction::triggered, canvas, &Canvas::undo);
    connect(ui->action_Redo, &QAction::triggered, canvas, &Canvas::redo);
    connect(ui->action_Delete, &QAction::triggered, canvas, &Canvas::deleteSelection);

    connect(new QShortcut(ui->action_Undo->shortcut(), canvas), &QShortcut::activated, canvas, &Canvas::undo);
    connect(new QShortcut(ui->action_Redo->shortcut(), canvas), &QShortcut::activated, canvas, &Canvas::redo);
    connect(new QShortcut(ui->action_Delete->shortcut(), canvas), &QShortcut::activated, canvas, &Canvas::deleteSelection);


    connect(new QShortcut(Qt::Key_Escape, canvas), &QShortcut::activated, canvas, &Canvas::cancel);


    connect(ui->actionSelect, &QAction::triggered, canvas, &Canvas::setSelect);
    connect(ui->actionPoints, &QAction::triggered, canvas, &Canvas::setPoints);
    connect(ui->actionLines, &QAction::triggered, canvas, &Canvas::setLines);

    connect(ui->actionRun, &QAction::triggered, this, &MainWindow::runClassifier);


    QActionGroup *group = new QActionGroup(this);
    group->addAction(ui->actionSelect);
    group->addAction(ui->actionPoints);
    group->addAction(ui->actionLines);

    ui->actionLines->setChecked(true);
    canvas->setMode(Lines);

    \
    config = std::shared_ptr<Config>(new Config());
    config->labels = {"background", "trunk"};

    int label = 0;
    for(std::string const& l : config->labels) {
        QListWidgetItem *item = new QListWidgetItem(l.c_str());
        item->setData(Qt::UserRole, QVariant(label++));
        ui->labelList->addItem(item);
    }

    ui->labelList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(ui->labelList, &QListWidget::currentRowChanged, canvas, &Canvas::setLabel);

    ui->labelList->setCurrentRow(1);



    nextImage();
}

void MainWindow::keyPressEvent(QKeyEvent *event) {

   size_t number = event->key() - '0';
   if(number >= 1 && number <= config->labels.size()) {
      int label = number - 1;
      ui->labelList->setCurrentRow(label);
      canvas->setLabel(label);
   }


   int amount = std::max<int>(1, ui->scaleSlider->value() * 0.1);
   if(event->key() == '-') {
       int value = ui->scaleSlider->value();
       ui->scaleSlider->setValue(value - amount);
   }

   if(event->key() == '+') {
       int value = ui->scaleSlider->value();
       ui->scaleSlider->setValue(value + amount);
   }


   if(event->key() == Qt::Key_Shift) {
       setCursor(Qt::BlankCursor);
   }


   QMainWindow::keyPressEvent(event);
}


void MainWindow::keyReleaseEvent(QKeyEvent *e) {
    if(e->key() == Qt::Key_Shift) {
        setCursor(Qt::ArrowCursor);
    }

    QMainWindow::keyReleaseEvent(e);
}



QString replaceExt(QString const &path, QString const &ext) {
    QFileInfo info(path);
    return QString (info.path() + "/" + info.completeBaseName() + ext);
}

inline cv::Mat1b loadMask(std::string const &path) {
    cv::Mat mask = cv::imread(path);

    if(mask.channels() > 1) {
        std::vector<cv::Mat> channels;
        cv::split(mask, channels);

        mask = channels[0];
    }

    return mask;
}

bool MainWindow::loadImage(QString const &path) {

    std::cout << "loading: " << path.toStdString() << std::endl;

    QPixmap p;
    if(p.load(path)) {
        filename = path;

        std::string maskPath = (path + ".mask").toStdString();
        cv::Mat1b mask = loadMask(maskPath);

        setWindowTitle(path);
        canvas->setImage(p, mask);


        return true;
    }

    return false;
}


void MainWindow::save() {
    if(currentEntry && canvas->isModified()) {
        QString temp = QDir::tempPath() + "/mask.png";
        QString labels = currentEntry->absoluteFilePath() + ".mask";

        cv::Mat1b mask = canvas->getMask();
        cv::imwrite(temp.toStdString(), mask);

        std::cout << temp.toStdString() << std::endl;

        QFile::remove(labels);
        QFile::rename(temp, labels);

        std::cout << "Writing " << labels.toStdString() << std::endl;

    }
}


void MainWindow::discardImage() {

    if(currentEntry) {
        QFile file(currentEntry->absoluteFilePath());
        QFile annot(currentEntry->absoluteFilePath() + ".json");
        QFile labels(currentEntry->absoluteFilePath() + ".mask");

        file.remove();
        annot.remove();
        labels.remove();

        loadNext(false);
    }
}

void MainWindow::nextImage() {
    save();
    loadNext(false);
}

void MainWindow::prevImage() {
    save();
    loadNext(true);
}

void MainWindow::runClassifier() {
    QProcess p;
    p.setWorkingDirectory("../segmenter");
    p.setProgram("python3.5");

    QString maskFile = QDir::currentPath() + "/.mask.png";
    p.setArguments({"test.py", filename, "--save", maskFile});

    p.start();

    p.waitForFinished();

    cv::Mat1b mask = loadMask(maskFile.toStdString());
    canvas->setMask(mask);
}

void MainWindow::loadNext(bool reverse) {

    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.PNG" << "*.jpeg" << "*.JPG" << "*.JPEG";


    QFileInfoList entries = path.entryInfoList(filters, QDir::Files|QDir::NoDotAndDotDot);
    if(reverse) std::reverse(entries.begin(), entries.end());
    int i = 0;

    if(currentEntry) {
        i = entries.indexOf(*currentEntry) + 1;
    }

    for(; i < entries.size(); ++i) {
        QFileInfo const &e = entries[i];

        QString name = e.filePath();
        QFileInfo annot (e.absoluteFilePath() + ".mask");

        if(ui->freshImage->isChecked() && annot.exists())
            continue;

        if(!ui->freshImage->isChecked() && !annot.exists())
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
