#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QListWidget>

#include <boost/optional.hpp>
#include <memory>
#include "state.h"
#include "canvas.h"

namespace Ui {
class MainWindow;
}


typedef boost::optional<QFileInfo> OptionalFileInfo;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    inline bool hasImage() { return bool(currentEntry); }

    ~MainWindow();

    bool open(QString const &path);
protected slots:


    void nextImage();
    void prevImage();
    void discardImage();

    void runClassifier();
    void runGrabCut();

    void setLabel(int label);



protected:

    bool save();
    bool loadNext(bool reverse = false);

    virtual void keyPressEvent(QKeyEvent *event);   
    virtual void keyReleaseEvent(QKeyEvent *e);

    virtual void closeEvent(QCloseEvent *e);

    void setImage(Image const &image);


    std::function<void(int)> setLayerOpacity(int layer) {
        return [=] (int opacity) {
            layers[layer]->setOpacity(opacity);
        };
    }


private:


    Ui::MainWindow *ui;


    std::shared_ptr<Config>  config;
    Canvas *canvas;
    std::vector<LayerPtr> layers;

    QString currentPath;
    Image currentImage;

    boost::optional<QFileInfo> currentEntry;
};

#endif // MAINWINDOW_H
