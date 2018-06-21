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
#include <opencv2/ximgproc.hpp>
#include <opencv2/imgcodecs.hpp>

inline QColor toColor(QJsonValue const &v) {
    auto c = v.toArray();
    return QColor(c[0].toInt(), c[1].toInt(), c[2].toInt(), c[3].toInt());
}

inline OptionalFileInfo findNext(QString const& path, Image& image, OptionalFileInfo const& current=OptionalFileInfo(), bool reverse=false, bool fresh=false);
inline cv::Mat1b loadMask(std::string const &path);
inline bool loadModel(QDir const& modelDir, Image &image);


inline std::shared_ptr<Config> loadConfig(QJsonObject const &root) {

    std::shared_ptr<Config> config (new Config());
    QJsonArray classes = root["classes"].toArray();

    auto ignore = root["ignored"].toObject();

    config->ignore_color = toColor(ignore["color"]);
    config->ignore_label = ignore["id"].toInt();

    config->default_label = root.contains("default") ? root["default"].toInt() : 0;

    for (int i = 0; i < classes.size(); ++i) {
        auto c = classes[i].toObject();

        Label label(c["name"].toString().toStdString(), i, toColor(c["color"]));
        config->labels.push_back(label);
    }

    return config;
}


std::shared_ptr<Config> readConfig(QString const &path) {

    std::shared_ptr<Config> config;;
    QFile file(path);

    std::cout << path.toStdString() << std::endl;

    if (file.open(QIODevice::ReadOnly)) {

        QByteArray data = file.readAll();
        QJsonDocument doc(QJsonDocument::fromJson(data));
        return loadConfig(doc.object());
    }

    return config;
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    ui->setupUi(this);

    canvas = new Canvas();
    layers = {LayerPtr(new Layer(0)), LayerPtr(new Layer(255))};
    canvas->setLayers(layers);


    ui->scrollArea->setEnabled(false);
    ui->scrollArea->setWidget(canvas);


    connect(ui->actionZoomOut, &QAction::triggered, canvas, &Canvas::zoomOut);
    connect(ui->actionZoomIn, &QAction::triggered, canvas, &Canvas::zoomIn);

    connect(ui->actionDiscard, &QAction::triggered, this, &MainWindow::discardImage);

    connect(ui->action_Next, &QAction::triggered, this, &MainWindow::nextImage);
    connect(ui->action_Prev, &QAction::triggered, this, &MainWindow::prevImage);


    connect(ui->action_Undo, &QAction::triggered, canvas, &Canvas::undo);
    connect(new QShortcut(ui->action_Undo->shortcut(), canvas), &QShortcut::activated, canvas, &Canvas::undo);

    connect(ui->action_Redo, &QAction::triggered, canvas, &Canvas::redo);
    connect(new QShortcut(ui->action_Redo->shortcut(), canvas), &QShortcut::activated, canvas, &Canvas::redo);

    connect(ui->action_Delete, &QAction::triggered, canvas, &Canvas::deleteSelection);
//    connect(new QShortcut(ui->action_Delete->shortcut(), canvas), &QShortcut::activated, canvas, &Canvas::deleteSelection);

    connect(new QShortcut(Qt::Key_Escape, canvas), &QShortcut::activated, canvas, &Canvas::cancel);


    connect(ui->actionSelect, &QAction::triggered, canvas, &Canvas::setSelect);
    connect(ui->actionPoints, &QAction::triggered, canvas, &Canvas::setPoints);
    connect(ui->actionLines, &QAction::triggered, canvas, &Canvas::setLines);
    connect(ui->actionFill, &QAction::triggered, canvas, &Canvas::setFill);
    //connect(ui->actionSuperPixels, &QAction::triggered, canvas, &Canvas::setSuperPixels);
    connect(ui->actionPolygons, &QAction::triggered, canvas, &Canvas::setPolygons);


    connect(ui->actionRefine, &QAction::toggled, [=](bool on) {
            canvas->setActiveLayer(on ? 1 : 0);
        });


    connect(ui->actionRun, &QAction::triggered, this, &MainWindow::runClassifier);
    //connect(ui->actionGrabCut, &QAction::triggered, this, &MainWindow::runGrabCut);


    connect(ui->brushWidth, &QSlider::valueChanged, canvas, &Canvas::setBrushWidth);
    connect(ui->labelOpacity, &QSlider::valueChanged, setLayerOpacity(0));
    connect(ui->refineOpacity, &QSlider::valueChanged, setLayerOpacity(1));


    connect(canvas, &Canvas::brushWidthChanged, ui->brushWidth, &QSlider::setValue);

//    connect(ui->spSize, &QSlider::valueChanged, canvas, &Canvas::setSPSize);
//    connect(ui->spSmoothness, &QSlider::valueChanged, canvas, &Canvas::setSPSmoothness);
    connect(ui->spOpacity, &QSlider::valueChanged, canvas, &Canvas::setOverlayOpacity);



//    canvas->setSPSize(ui->spSize->value());
//    canvas->setSPSmoothness(ui->spSmoothness->value());
    canvas->setOverlayOpacity(ui->spOpacity->value());

    setLayerOpacity(0)(ui->labelOpacity->value());
    setLayerOpacity(1)(ui->refineOpacity->value());


    canvas->setBrushWidth(ui->brushWidth->value());



    ui->labelList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(ui->labelList, &QListWidget::currentRowChanged, this, &MainWindow::setLabel);

    ui->labelList->setFocusPolicy(Qt::NoFocus);

    QActionGroup *group = new QActionGroup(this);
    group->addAction(ui->actionSelect);
    group->addAction(ui->actionPoints);
    group->addAction(ui->actionLines);
    group->addAction(ui->actionFill);
    group->addAction(ui->actionSuperPixels);

    ui->actionPoints->setChecked(true);
    canvas->setMode(Points);
}

inline QListWidgetItem *makeLabel(Label const &label) {

    QPixmap p(16, 16);
    p.fill(label.color);

    QListWidgetItem *item = new QListWidgetItem(label.name.c_str());
    item->setData(Qt::UserRole, QVariant(label.value));
    item->setIcon(QIcon(p));

    return item;
}


inline QVector<QRgb> configColorTable(Config const &c) {
    QVector<QRgb> palette(256, c.ignore_color.rgba());

    int i = 0;
    for (Label const& l : c.labels) {
        QColor c = l.color;
        palette[i++] = c.rgb();
    }

    return palette;
}


inline QVector<QRgb> setTransparent(QVector<QRgb> const &table, int transparent=0) {

    QVector<QRgb> t = table;
    if(transparent < t.size()) {
        QColor c = t[transparent];
        c.setAlpha(0);

        t[transparent] = c.rgba();
    }

    return t;
}

bool MainWindow::open(QString const &path) {

    auto newConfig = readConfig(path + "/config.json");
    if(!newConfig) newConfig = readConfig(path + "/../config.json");

    if(!newConfig) {
        QMessageBox::warning(this, "Open", "Could not find config file in directory (or parent): " + path);
        return false;
    }

    Image image;
    OptionalFileInfo next = findNext(path, image, OptionalFileInfo(), false, ui->actionFresh->isChecked());
    if(!next) next = findNext(path, image, OptionalFileInfo(), false, !ui->actionFresh->isChecked());


    if(!next) {
        QMessageBox::warning(this, "Open", "No valid images found in: " + path);
        return false;
    }

    config = newConfig;
    currentPath = path;
    currentEntry = next;

    ui->labelList->clear();

    for(auto& label: config->labels)
        ui->labelList->addItem(makeLabel(label));

    Label ignoreLabel("ignored", config->ignore_label, config->ignore_color);
    ui->labelList->addItem(makeLabel(ignoreLabel));
    layers[1]->setDefaultLabel(ignoreLabel.value);


    ui->labelList->setCurrentRow(0);

    QVector<QRgb> colorTable = configColorTable(*config);
    layers[0]->setPalette(setTransparent(colorTable, 0));
    layers[1]->setPalette(setTransparent(colorTable, ignoreLabel.value));

    layers[1]->setDefaultLabel(ignoreLabel.value);

    setImage(image);
    this->setWindowTitle(next->fileName());

    ui->scrollArea->setEnabled(true);
    return true;
}


void MainWindow::setImage(Image const &loaded) {
    canvas->setImage(loaded.image);

    if(!loaded.prediction.empty())
        layers[0]->setMask(loaded.prediction);

    if(!loaded.labels.empty())
        layers[1]->setMask(loaded.labels);

    currentImage = loaded;
}


void MainWindow::setLabel(int label) {
    if(label < int(config->labels.size())) {
        canvas->setLabel(config->labels[label].value);
    } else {
        canvas->setLabel(config->ignore_label);
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event) {

   if(config) {
       int number = event->key() - '1';

       if(number >= 0 && number < int(config->labels.size())) {

          int label = number;
          ui->labelList->setCurrentRow(label);

          canvas->setLabel(config->labels[label].value);
       }

       if(number == -1) {
           ui->labelList->setCurrentRow(config->labels.size());
           canvas->setLabel(config->ignore_label);
       }
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
        QString labelFile = currentEntry->absoluteFilePath() + ".mask";


        cv::Mat1b mask = canvas->getActiveLayer()->getMask();
        cv::imwrite(temp.toStdString(), mask);

        std::cout << temp.toStdString() << std::endl;


        QFile::remove(labelFile);
        QFile::rename(temp, labelFile);

        std::vector<Event> log = canvas->getLog();
        QJsonArray events;

        for (auto const &e : log) {
            QJsonObject obj;
            obj["time"] = e.time;
            obj["event"] = QString(e.event.c_str());

            events.append(obj);
        }

        QJsonDocument doc(events);
        QFile logFile (currentEntry->absoluteFilePath() + ".log");

        if (logFile.open(QIODevice::WriteOnly)) {
            logFile.write(doc.toJson());
         }



        std::cout << "Writing " << labelFile.toStdString() << std::endl;
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

void MainWindow::runGrabCut() {
//    canvas->grabCut();

}


void MainWindow::runClassifier() {
    QProcess p;
    p.setWorkingDirectory("../segmenter");
    p.setProgram("python3");

    //QString maskFile = QDir::currentPath() + "/.mask.png";

    QString file = currentEntry->absoluteFilePath();
    QString maskDir = file + ".model";

    p.setArguments({"test.py", "--image", file, "--save", maskDir, "--model", currentPath + "/log/train/model.pth"});

    p.start();
    p.waitForFinished();

    if(p.exitCode() == 0) {
        loadModel(maskDir, currentImage);
        assert (!currentImage.prediction.empty());

        layers[0]->setMask(currentImage.prediction);

    } else {
        QString perr = p.readAllStandardError();
        QMessageBox::critical(this, "Classifier", perr);
    }
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


inline bool loadModel(QDir const& modelDir, Image &image) {

    if(modelDir.exists()) {
        std::string maskPath = (modelDir.path() + "/predictions.png").toStdString();


        std::cout << maskPath << std::endl;

        image.prediction = loadMask(maskPath);

        int i = 0;
        while(true) {
            std::ostringstream probPath;
            probPath << modelDir.path().toStdString() << "/class" << i++ << ".jpg";


            cv::Mat1b prob = cv::imread(probPath.str(), cv::IMREAD_GRAYSCALE);
            if(!prob.empty()) {
                image.probs.push_back(prob);
            } else {
                break;
            }
        }

        return true;
    }

    return false;
}


inline bool loadImage(QString const &path, Image &image) {

    std::cout << "loading: " << path.toStdString() << std::endl;
    image.image = cv::imread(path.toStdString(), cv::IMREAD_COLOR);

    if(!image.image.empty()) {
        cv::cvtColor(image.image, image.image, cv::COLOR_BGR2RGB);

        QDir modelDir(path + ".model");
            loadModel(modelDir, image);


        std::string maskPath = (path + ".mask").toStdString();
        cv::Mat1b annotated = loadMask(maskPath);
        if(!annotated.empty()) {
            image.labels = annotated;
        }

        return true;
    }

    return false;
}

inline OptionalFileInfo findNext(QString const& path, Image& image, OptionalFileInfo const& current, bool reverse, bool fresh) {
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.PNG" << "*.jpeg" << "*.JPG" << "*.JPEG";

    QDir dir(path);

    QFileInfoList entries = dir.entryInfoList(filters, QDir::Files|QDir::NoDotAndDotDot);
    if(reverse) std::reverse(entries.begin(), entries.end());
    int i = 0;

    if(current) {
        i = entries.indexOf(*current) + 1;
    }

    for(; i < entries.size(); ++i) {
        QFileInfo const &e = entries[i];

        QString name = e.filePath();
        QFileInfo annot (e.absoluteFilePath() + ".mask");

//        if(fresh && annot.exists())
//            continue;

//        if(!fresh && !annot.exists())
//            continue;

        QPixmap p;
        if(loadImage(name, image)) {
            return e;
        }
    }

    return boost::optional<QFileInfo>();
}


bool MainWindow::loadNext(bool reverse) {

    Image loaded;
    auto next = findNext(currentPath, loaded, currentEntry, reverse, ui->actionFresh->isChecked());
    if(next) {
        this->setWindowTitle(next->fileName());
        setImage(loaded);


        currentEntry = next;
    }

    return bool(next);
}


MainWindow::~MainWindow()
{
    delete ui;
}
