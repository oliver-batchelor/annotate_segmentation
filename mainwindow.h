#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>

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
    explicit MainWindow(QWidget *parent = 0);

    bool nextImage(QDir const &path);
    bool loadImage(QString const &path);

    ~MainWindow();

private:
    Ui::MainWindow *ui;

    std::shared_ptr<State> state;
    Canvas *canvas;
};

#endif // MAINWINDOW_H
