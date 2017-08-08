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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QDir const& path, QWidget *parent = 0);
    inline bool hasImage() { return bool(currentEntry); }

    ~MainWindow();

protected slots:

    void nextImage();
    void prevImage();
    void discardImage();

    void runClassifier();
    void setLabel(int label);



protected:

    bool save();
    void loadNext(bool reverse = false);

    virtual void keyPressEvent(QKeyEvent *event);   
    virtual void keyReleaseEvent(QKeyEvent *e);

    virtual void closeEvent(QCloseEvent *e);

    bool loadImage(QString const &path);


private:


    Ui::MainWindow *ui;


    std::shared_ptr<Config>  config;
    QString filename;

    Canvas *canvas;

    QDir path;
    boost::optional<QFileInfo> currentEntry;
};

#endif // MAINWINDOW_H
