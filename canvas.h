#ifndef CANVAS_H
#define CANVAS_H


#include <QWidget>
#include "state.h"

enum DrawState {
    Waiting,
    First,
    Second
};

class Canvas : public QWidget
{
    Q_OBJECT

public:
    Canvas(std::shared_ptr<State> _state);
    void setImage(QPixmap const& p);

protected:
    void setLast(QPointF const &p);


    //    bool event(QEvent *event);
    //void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    //    void mouseReleaseEvent(QMouseEvent *event);

private:

    std::shared_ptr<State> state;
    DrawState drawState;

    Area progress;
    QPixmap image;
};

#endif // CANVAS_H
