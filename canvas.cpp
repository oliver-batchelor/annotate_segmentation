#include "canvas.h"
#include <QPainter>
#include <QMouseEvent>

#include <iostream>

Canvas::Canvas(std::shared_ptr<State> state)
        : state(state), drawState(Waiting), scale(1.0f) {
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



inline bool backward(QLineF const &p, QLineF const &n) {
    return QLineF (p.p1(), n.p1()).intersect(QLineF (p.p2(), n.p2()), NULL) == QLineF::BoundedIntersection;
}

inline QLineF flip(QLineF &l) {
    return QLineF(l.p2(), l.p1());
}

QPolygonF makeQuad(QLineF p, QLineF n) {
    QVector<QPointF> v = {p.p1(), n.p1(), n.p2(), p.p2()};
    return QPolygonF (v);
}

Rect makeRect(QLineF p, QLineF n, int label) {
    if(backward(p, n)) n = flip(n);
    return Rect(p, n, label);
}

inline void drawRect(Rect const &r, QPainter *painter) {

    std::cout << r.label << std::endl;
    QColor c ((Qt::GlobalColor)(r.label + 8));
    c.setAlpha(127);
    painter->setBrush(c);

    painter->drawPolygon(makeQuad(r.side1, r.side2));
}

inline void drawArea(QPainter *painter, Area const &area) {




    for(Rect const &r : area.sections) {
        drawRect(r, painter);
    }

}

void Canvas::zoom(float level) {
    scale = level / 100;

    if(!image.isNull()) {
        scaled = image.scaled(image.size() * scale);
    } else {
        scaled = QPixmap();
    }

    resize(scaled.size());

}

void Canvas::setImage(QPixmap const &p) {

    image = p;
    zoom(scale * 100);
}

void Canvas::setLabel(int label) {
    currentLabel = label;
    repaint();
}


void Canvas::setPoint(QPointF const &p) {

    switch(drawState) {
    case First: currentSection.setP1(p);
    case Second: currentSection.setP2(p);
    default: break;
    }

}



void Canvas::mousePressEvent(QMouseEvent *event) {
    QPointF p(event->x() / scale, event->y() / scale);


    if(event->button() == Qt::LeftButton) {
        switch(drawState) {
        case Waiting:
            currentSection = QLineF(p, p);
            drawState = Second;
        break;

        case First:
            currentSection = QLineF(p, p);
            drawState = Second;
        break;

        case Second:
            currentSection.setP2(p);

            if(lastSection) {
                Rect r = makeRect(*lastSection, currentSection, currentLabel);
                currentArea.sections.push_back(r);
            }

            lastSection = currentSection;
            currentSection = QLineF(p, p);

            drawState = First;

        break;
        }
    } else {

       if(drawState != Waiting) {

           if(currentArea.sections.size())
               state->areas.push_back(currentArea);

           currentArea.sections.clear();
        }

       lastSection.reset();
       drawState = Waiting;
    }

    this->repaint();
}

void Canvas::mouseReleaseEvent(QMouseEvent *event) {
    QPointF p(event->x() / scale, event->y() / scale);

    if(selection) {

        selection->setBottomRight(p);
        selection.reset();


    }
}

void Canvas::mouseMoveEvent(QMouseEvent *event) {
    QPointF p(event->x() / scale, event->y() / scale);
    setPoint(p);

    this->repaint();
}

void Canvas::paintEvent(QPaintEvent * /* event */) {
    QPainter painter(this);

    painter.fillRect(rect(), QColor(Qt::gray));
    painter.drawPixmap(0, 0, scaled);

    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.scale(scale, scale);
    \
    QPen pen(Qt::black);
    pen.setWidth(2 / scale);

    painter.setPen(pen);


    for(auto a : state->areas) {
        drawArea(&painter, a);
    }

    drawArea(&painter, currentArea);

    if(lastSection) {
        std::cout << currentLabel << std::endl;
        drawRect(makeRect(*lastSection, currentSection, currentLabel), &painter);
    }

    if(drawState != Waiting) {
        painter.drawLine(currentSection);
    }

    if(selection) {
        painter.drawRect(*selection);
    }

}
