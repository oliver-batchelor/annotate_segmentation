#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>

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

protected:

    virtual void keyPressEvent(QKeyEvent *event);   
    bool loadImage(QString const &path);


private:


    Ui::MainWindow *ui;

    std::shared_ptr<State> state;
    std::shared_ptr<Config>  config;
    Canvas *canvas;

    QDir path;
    boost::optional<QFileInfo> currentEntry;
};

#endif // MAINWINDOW_H
