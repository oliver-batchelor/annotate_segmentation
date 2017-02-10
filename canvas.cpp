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


inline perpLine(QPointF const &start, QPointF const &end) {
    QPointF line = end - start;
    QPointF dir = line / length(line);

    return QPointF (dir.y(), -dir.x());
}


QPolygonF areaPoly(Area const &area) {

    QVector<QPointF> v;

//    for(size_t i = 0; i < area.sections.size(); ++i) {
//        QPointF perp =
//            (i > 0 ? segs area.sections[i - 1] : QPointF(0, 0)) +
//            (i < area.sections ? area.sections[i - 1] : QPointF(0, 0))
//                ;


//    }

    QPointF perp = perpLine(area.start, area.end) * area.width;


    = { area.start - perp, area.start + perp, area.end + perp, area.end - perp };
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

        case Width:
            QPointF

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
