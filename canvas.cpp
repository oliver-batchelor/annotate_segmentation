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




inline void drawLine(Line const &l, QPainter *painter) {

    std::cout << r.label << std::endl;
    QColor c ((Qt::GlobalColor)(r.label + 8));
    c.setAlpha(127);
    painter->setBrush(c);

    painter->drawPolygon(makeQuad(r.side1, r.side2));
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

Line Canvas::currentLine() {
    return Line (lastPoint ? *lastPoint : currentPoint, currentPoint, currentLabel);
}


void Canvas::mousePressEvent(QMouseEvent *event) {
    currentPoint.p = QPointF(event->x() / scale, event->y() / scale);

    if(event->button() == Qt::LeftButton) {

        if(lastPoint) {
            currentArea.sections.push_back(currentLine());
        }

        lastPoint = currentPoint;
    } else {

        if(lastPoint) {
            currentArea.sections.push_back(currentLine());
        }

        state->areas.push_back(currentArea);
        currentArea.clear();

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
    currentPoint.p = QPointF(event->x() / scale, event->y() / scale);
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

    if(currentArea.size())
        drawArea(currentArea);
    else
        drawLine(currentLine());


    if(selection) {
        painter.drawRect(*selection);
    }

}
