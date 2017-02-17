#ifndef CANVAS_H
#define CANVAS_H


#include <QWidget>
#include "state.h"

#include <boost/optional.hpp>

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

public slots:
    void zoom(float zoom);
    void setLabel(int label);


protected:
    void setPoint(QPointF const &p);

    //    bool event(QEvent *event);
    //void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:

    std::shared_ptr<State> state;
    DrawState drawState;

    boost::optional<QLineF> lastSection;
    QLineF currentSection;

    boost::optional<QRectF> selection;


    int currentLabel;
    Area currentArea;

    QPixmap image;
    QPixmap scaled;

    float scale;
};

#endif // CANVAS_H
