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
#include <QMessageBox>

#include <iostream>
#include <fstream>

#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>


std::vector<Label> readClasses(std::string const &path) {

    std::ifstream in(path);
    assert(in.is_open() && "could not open classes.txt");

    std::vector<Label> classes;

    int n = 0;
    std::string line;
    while (std::getline(in, line)) {
        classes.push_back(Label(line, n++));
    }

    return classes;
}


MainWindow::MainWindow(QDir const &path, QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    path(path)
{

    ui->setupUi(this);

    canvas = new Canvas();
    ui->scrollArea->setWidget(canvas);

    connect(ui->actionZoomIn, &QAction::triggered, canvas, &Canvas::zoomIn);
    connect(ui->actionZoomOut, &QAction::triggered, canvas, &Canvas::zoomOut);

    connect(ui->actionDiscard, &QAction::triggered, this, &MainWindow::discardImage);

    connect(ui->action_Next, &QAction::triggered, this, &MainWindow::nextImage);
    connect(ui->action_Prev, &QAction::triggered, this, &MainWindow::prevImage);


    connect(ui->action_Undo, &QAction::triggered, canvas, &Canvas::undo);
    connect(new QShortcut(ui->action_Undo->shortcut(), canvas), &QShortcut::activated, canvas, &Canvas::undo);

    connect(ui->action_Redo, &QAction::triggered, canvas, &Canvas::redo);
    connect(new QShortcut(ui->action_Redo->shortcut(), canvas), &QShortcut::activated, canvas, &Canvas::redo);

    connect(ui->action_Delete, &QAction::triggered, canvas, &Canvas::deleteSelection);
    connect(new QShortcut(ui->action_Delete->shortcut(), canvas), &QShortcut::activated, canvas, &Canvas::deleteSelection);
    connect(new QShortcut(Qt::Key_Escape, canvas), &QShortcut::activated, canvas, &Canvas::cancel);


    connect(ui->actionSelect, &QAction::triggered, canvas, &Canvas::setSelect);
    connect(ui->actionPoints, &QAction::triggered, canvas, &Canvas::setPoints);
    connect(ui->actionLines, &QAction::triggered, canvas, &Canvas::setLines);
    connect(ui->actionFill, &QAction::triggered, canvas, &Canvas::setFill);
    connect(ui->actionSuperPixels, &QAction::triggered, canvas, &Canvas::setSuperPixels);

    connect(ui->actionRun, &QAction::triggered, this, &MainWindow::runClassifier);

    connect(ui->brushWidth, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), canvas, &Canvas::setBrushWidth);
    connect(canvas, &Canvas::brushWidthChanged, ui->brushWidth, &QSpinBox::setValue);

    connect(ui->spSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), canvas, &Canvas::setSPSize);
    connect(ui->spSmoothness, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), canvas, &Canvas::setSPSmoothness);

    canvas->setSPSize(ui->spSize->value());
    canvas->setSPSmoothness(ui->spSmoothness->value());


    QActionGroup *group = new QActionGroup(this);
    group->addAction(ui->actionSelect);
    group->addAction(ui->actionPoints);
    group->addAction(ui->actionLines);
    group->addAction(ui->actionFill);
    group->addAction(ui->actionSuperPixels);

    ui->actionPoints->setChecked(true);
    canvas->setMode(Points);

    config = std::shared_ptr<Config>(new Config());
    config->labels = readClasses((path.path() + "/classes.txt").toStdString());

    config->labels.push_back(Label("ignored", 255));

    for(auto& label: config->labels) {
        QListWidgetItem *item = new QListWidgetItem(label.name.c_str());
        item->setData(Qt::UserRole, QVariant(label.value));
        ui->labelList->addItem(item);
    }

    ui->labelList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(ui->labelList, &QListWidget::currentRowChanged, this, &MainWindow::setLabel);

    ui->labelList->setCurrentRow(0);
    nextImage();

    if(!currentEntry) {
        ui->actionFresh->setChecked(true);
        nextImage();
    }

}


void MainWindow::setLabel(int label) {
    assert(label < int(config->labels.size()) && "label out of range");
    canvas->setLabel(config->labels[label].value);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {

   size_t number = event->key() - '0';
   if(number >= 1 && number < config->labels.size()) {
      std::cout << "asdfasdfafds" << number << std::endl;

      int label = number - 1;
      ui->labelList->setCurrentRow(label);

      canvas->setLabel(config->labels[label].value);
   }

   if(number == 0) {
       ui->labelList->setCurrentRow(config->labels.size() - 1);
       canvas->setLabel(config->labels.back().value);
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



void MainWindow::closeEvent(QCloseEvent *e) {
    if(save()) e->accept();
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
    cv::Mat3b image = cv::imread(path.toStdString(), cv::IMREAD_COLOR);


    if(!image.empty()) {
        cv::cvtColor(image, image, cv::COLOR_BGR2RGB);

        filename = path;

        std::string maskPath = (path + ".mask").toStdString();
        cv::Mat1b mask = loadMask(maskPath);

        setWindowTitle(path);
        canvas->setImage(image, mask);

        return true;
    }

    return false;
}


bool MainWindow::save() {
    if(currentEntry && canvas->isModified()) {


        if(!ui->actionAlwaysSave->isChecked()) {
            QMessageBox::StandardButton button = QMessageBox::question(this, "Save", "Save changes before closing?",
                                        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel);

            if(button == QMessageBox::Cancel)
                return false;

            if(button == QMessageBox::No)
                return true;
        }


        QString temp = QDir::tempPath() + "/mask.png";
        QString labels = currentEntry->absoluteFilePath() + ".mask";

        cv::Mat1b mask = canvas->getMask();
        cv::imwrite(temp.toStdString(), mask);

        std::cout << temp.toStdString() << std::endl;

        QFile::remove(labels);
        QFile::rename(temp, labels);

        std::cout << "Writing " << labels.toStdString() << std::endl;
    }

    return true;
}


void MainWindow::discardImage() {
    if(currentEntry && QMessageBox::Yes == QMessageBox::warning(this, "Discard image", "Are you sure you wish to permanantly delete image and annotations?",
                                                                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No)) {


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
    if(save()) loadNext(false);
}

void MainWindow::prevImage() {
    if(save()) loadNext(true);
}

void MainWindow::runClassifier() {
    QProcess p;
    p.setWorkingDirectory("../segmenter");
    p.setProgram("python3.5");

    QString maskFile = QDir::currentPath() + "/.mask.png";
    QFile::remove(maskFile);

    p.setArguments({"test.py", filename, "--save", maskFile});

    p.start();
    p.waitForFinished();

    if(p.exitCode() == 0) {

        cv::Mat1b mask = loadMask(maskFile.toStdString());
        canvas->setMask(mask);
    } else {
        QString perr = p.readAllStandardError();
        QMessageBox::critical(this, "Classifier", perr);
    }
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

        if(ui->actionFresh->isChecked() && annot.exists())
            continue;

        if(!ui->actionFresh->isChecked() && !annot.exists())
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
