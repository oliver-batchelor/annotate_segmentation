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


#include <iostream>

MainWindow::MainWindow(QDir const &path, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    path(path)
{
    ui->setupUi(this);

    state = std::shared_ptr <State>(new State());
    canvas = new Canvas(state);\
    ui->scrollArea->setWidget(canvas);

    connect(ui->scaleSlider, &QSlider::valueChanged, canvas, &Canvas::zoom);
    connect(ui->nextImage, &QPushButton::clicked, this, &MainWindow::nextImage);

    connect(ui->prevImage, &QPushButton::clicked, this, &MainWindow::prevImage);

    connect(ui->action_Next, &QAction::triggered, this, &MainWindow::nextImage);
    connect(ui->action_Prev, &QAction::triggered, this, &MainWindow::prevImage);


    connect(ui->action_Undo, &QAction::triggered, canvas, &Canvas::undo);
    connect(ui->action_Redo, &QAction::triggered, canvas, &Canvas::redo);
    connect(ui->action_Delete, &QAction::triggered, canvas, &Canvas::deleteSelection);



    config = std::shared_ptr<Config>(new Config());
    config->labels = {"trunk"};

    int label = 1;
    for(std::string const& l : config->labels) {
        QListWidgetItem *item = new QListWidgetItem(l.c_str());
        item->setData(Qt::UserRole, QVariant(label++));
        ui->labelList->addItem(item);
    }

    ui->labelList->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->labelList->setCurrentRow(0);

    canvas->setLabel(1);


    nextImage();
}

void MainWindow::keyPressEvent(QKeyEvent *event) {

   size_t number = event->key() - '0';
   if(number >= 1 && number <= config->labels.size()) {
      int label = number;
      ui->labelList->setCurrentRow(label - 1);

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

   QMainWindow::keyPressEvent(event);
}




Point readPoint(const QJsonObject &json)
{
    QPointF p(json["x"].toDouble(), json["y"].toDouble());
    return Point(p, json["r"].toDouble());
}

QJsonObject writePoint(Point const& p) {
    QJsonObject obj;

    obj["x"] = p.p.x();
    obj["y"] = p.p.y();
    obj["r"] = p.r;

    return obj;
}

Area readArea(const QJsonObject &json) {
    Area a;
    a.label = json["label"].toInt();

    QJsonArray arr = json["line"].toArray();
    for(QJsonValue const &v : arr) {
        a.line.push_back(readPoint(v.toObject()));
    }

    return a;
}

QJsonObject writeArea(Area const& a) {
    QJsonObject json;

    json["label"] = a.label;

    QJsonArray arr;
    for(Point const &p : a.line) {
        arr.append(writePoint(p));
    }
    json["line"] = arr;

    return json;
}


State readState(const QJsonDocument &doc) {
    State s;

    QJsonObject json = doc.object();
    s.filename = json["filename"].toString();

    QJsonArray arr = json["areas"].toArray();
    for(QJsonValue const &v : arr) {
        s.areas.push_back(readArea(v.toObject()));
    }

    return s;
}

QJsonDocument writeState(State const& s) {
    QJsonObject json;

    json["filename"] = s.filename;

    QJsonArray arr;
    for(Area const &a : s.areas) {
        arr.append(writeArea(a));
    }
    json["areas"] = arr;

    return QJsonDocument(json);
}




QString replaceExt(QString const &path, QString const &ext) {
    QFileInfo info(path);
    return QString (info.path() + "/" + info.completeBaseName() + ext);
}

bool MainWindow::loadImage(QString const &path) {

    std::cout << "loading: " << path.toStdString() << std::endl;

    QPixmap p;
    if(p.load(path)) {
        state->areas.clear();
        state->filename = path;

        QFileInfo annot(replaceExt(path, ".json"));
        if(annot.isFile() && annot.isReadable()) {
            QFile file(annot.absoluteFilePath());
            file.open(QIODevice::ReadOnly);

            QByteArray saveData = file.readAll();
            State s = readState(QJsonDocument::fromJson(saveData));
            state->areas = s.areas;
        }

        setWindowTitle(path);
        canvas->setImage(p);

        return true;
    }

    return false;
}


void MainWindow::save() {
    if(currentEntry && state->areas.size() && canvas->isModified()) {
        QFileInfo annot(replaceExt(currentEntry->absoluteFilePath(), ".json"));
        QFileInfo labels(replaceExt(currentEntry->absoluteFilePath(), ".mask"));

        QFile file(annot.absoluteFilePath());
        file.open(QIODevice::WriteOnly);

        file.write(writeState(*state).toJson());

        QImage mask = canvas->save();
        mask.save(labels.absoluteFilePath(), "png");
    }
}


void MainWindow::nextImage() {
    loadNext(false);
}

void MainWindow::prevImage() {
    loadNext(true);
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
        QFileInfo annot (replaceExt(e.absoluteFilePath(), ".json"));

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
