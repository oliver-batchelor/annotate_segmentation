#include "canvas.h"
#include <QPainter>
#include <QMouseEvent>

#include <iostream>

Canvas::Canvas(std::shared_ptr<State> state)
        : state(state), drawState(Start) {
    setMouseTracking(true);
}

float length(QPointF const &p) {
    return std::sqrt(QPointF::dotProduct(p, p));
}



QPolygonF areaPoly(Area const &area) {

    QPointF line = area.end - area.start;
    QPointF dir = line / length(line);

    QPointF perp (dir.y(), -dir.x());
    perp *= area.width;

    QVector<QPointF> v = { area.start - perp, area.start + perp, area.end + perp, area.end - perp };
    return QPolygonF(v);
}

inline void drawArea(QPainter *painter, Area const &area) {

    painter->save();

    QPen pen(Qt::black);
    pen.setWidth(2);

    painter->setPen(pen);

    QColor c ((Qt::GlobalColor)area.label);
    painter->setBrush(c);
    painter->drawPolygon(areaPoly(area));

    painter->restore();
}

void Canvas::mousePressEvent(QMouseEvent *event) {

    std::cout << drawState << std::endl;

    if(event->button() == Qt::LeftButton) {
        switch(drawState) {
            case Start:
                progress.start = QPointF(event->x(), event->y());
                progress.end = progress.start;
                drawState = End;
            break;

            case End:
                drawState = Width;
            break;


            default:
            break;

        }
    } else {
       drawState = Start;
    }

    this->repaint();

}

void Canvas::mouseMoveEvent(QMouseEvent *event) {
    switch(drawState) {
        case End:
            progress.end = QPointF(event->x(), event->y());
        break;
        default:
        break;

    }

    this->repaint();
}

void Canvas::paintEvent(QPaintEvent * /* event */) {
    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing, true);

    QPen pen(Qt::black);
    pen.setWidth(5);

    painter.setPen(pen);

    switch(drawState) {
        case Width:
        case End:
            painter.drawPoint(progress.start);
            painter.drawPoint(progress.end);
            pen.setWidth(2);
            pen.setStyle(Qt::DashDotLine);
            painter.setPen(pen);

            painter.drawLine(progress.start, progress.end);

        break;


        default:
        break;

    }


    for(auto a : state->areas) {
        drawArea(&painter, a);
    }


}
