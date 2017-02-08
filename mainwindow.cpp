#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "canvas.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    Canvas *canvas = new Canvas();

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(canvas);

    ui->holder->setLayout(layout);
}

MainWindow::~MainWindow()
{
    delete ui;
}
