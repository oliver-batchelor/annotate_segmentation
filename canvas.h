#ifndef CANVAS_H
#define CANVAS_H


#include <QWidget>


#incude "state.h"

class Canvas : public QWidget
{
    Q_OBJECT

public:
    Canvas(std::shared_ptr<State> _state);


protected:
//    bool event(QEvent *event);
//    void resizeEvent(QResizeEvent *event);
void paintEvent(QPaintEvent *event);
//    void mousePressEvent(QMouseEvent *event);
//    void mouseMoveEvent(QMouseEvent *event);
//    void mouseReleaseEvent(QMouseEvent *event);

private:

    std::shared_ptr<State> state;
};

#endif // CANVAS_H
