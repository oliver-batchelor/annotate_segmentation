#ifndef CANVAS_H
#define CANVAS_H


#include <QWidget>
#include "state.h"

enum DrawState {
    Start,
    End,
    Width
};

class Canvas : public QWidget
{
    Q_OBJECT

public:
    Canvas(std::shared_ptr<State> _state);


protected:
    //    bool event(QEvent *event);
    //    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    //    void mouseReleaseEvent(QMouseEvent *event);

private:

    std::shared_ptr<State> state;
    DrawState drawState;

    Area progress;
};

#endif // CANVAS_H
