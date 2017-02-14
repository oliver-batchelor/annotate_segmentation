#include "canvas.h"
#include <QPainter>
#include <QMouseEvent>

#include <iostream>

Canvas::Canvas(std::shared_ptr<State> state)
        : state(state), drawState(Waiting) {
    setMouseTracking(true);
}

float length(QPointF const &p) {
    return std::sqrt(QPointF::dotProduct(p, p));
}


inline QPointF perpLine(QPointF const &start, QPointF const &end) {
    QPointF line = end - start;
    QPointF dir = line / length(line);

    return QPointF (dir.y(), -dir.x());
}


//QPolygonF areaPoly(Area const &area) {

//    QVector<QPointF> points;

//    for(QLineF const& s : area.sections) {

//        bool backward = false;
//        if(points.size()) {
//            QLineF l1 (points.back(), s.p2());
//            QLineF l2 (points.front(), s.p1());

//            backward = l1.intersect(l2, NULL);
//        }

//        std::cout << backward << std::endl;

//        if(!backward) {
//            points.push_front(s.p1());
//            points.push_back(s.p2());
//        } else {
//            points.push_front(s.p2());
//            points.push_back(s.p1());
//        }
//    }

//    return QPolygonF(points);
//}

inline bool backward(QLineF const &p, QLineF const &n) {
    return QLineF (p.p1(), n.p1()).intersect(QLineF (p.p2(), n.p2()), NULL) == QLineF::BoundedIntersection;
}

inline QLineF flip(QLineF &l) {
    return QLineF(l.p2(), l.p1());
}

QPolygonF makeQuad(QLineF p, QLineF n) {
    if(backward(p, n)) {
        n = flip(n);
    }

    QVector<QPointF> v = {p.p1(), n.p1(), n.p2(), p.p2()};
    return QPolygonF (v);
}

inline void drawArea(QPainter *painter, Area const &area) {

    painter->save();

    QPen pen(Qt::black);
    pen.setWidth(2);

    painter->setPen(pen);

    QColor c ((Qt::GlobalColor)area.label + 7);
    c.setAlpha(127);
    painter->setBrush(c);

    for(size_t i = 1; i < area.sections.size(); ++i) {
        painter->drawPolygon(makeQuad(area.sections[i - 1], area.sections[i]));
    }

    painter->restore();
}

void Canvas::setImage(QPixmap const &p) {

    image = p;


}

void Canvas::setLast(QPointF const &p) {

    QLineF &last = progress.sections.back();

    switch(drawState) {
    case First: last.setP1(p);
    case Second: last.setP2(p);
    default: break;
    }



}

void Canvas::mousePressEvent(QMouseEvent *event) {
    QPointF p(event->x(), event->y());


    if(event->button() == Qt::LeftButton) {
        switch(drawState) {
        case Waiting:
            progress.label = state->currentLabel;

            progress.sections.clear();
            progress.sections.push_back(QLineF(p, p));

            drawState = Second;
        break;

        case First:
            setLast(p);
            drawState = Second;
        break;

        case Second:
            setLast(p);

            progress.sections.push_back(QLineF(p, p));
            drawState = First;

        break;
        }
    } else {

       if(drawState != Waiting) {
           progress.sections.pop_back();
           if(progress.sections.size() >= 2) {
               state->areas.push_back(progress);
               progress.sections.clear();
           }
        }

        drawState = Waiting;
    }

    this->repaint();

}

void Canvas::mouseMoveEvent(QMouseEvent *event) {
    QPointF p(event->x(), event->y());
    setLast(p);

    this->repaint();
}

void Canvas::paintEvent(QPaintEvent * /* event */) {
    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(Qt::gray));



    if(progress.sections.size()) {
        drawArea(&painter, progress);
    }



    for(auto a : state->areas) {
        drawArea(&painter, a);
    }


}
