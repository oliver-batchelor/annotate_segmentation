#include "canvas.h"
#include <QPainter>
#include <QMouseEvent>

#include <iostream>

Canvas::Canvas(std::shared_ptr<State> state)
        : state(state), currentWidth(20.0f), scale(1.0f) {
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




inline void drawArea(Area const &a, QPainter *painter) {

    QColor c ((Qt::GlobalColor)(a.label + 8));
    c.setAlpha(127);
    painter->setBrush(c);

   // painter->drawPolygon(makeQuad(r.side1, r.side2));
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




void Canvas::mousePressEvent(QMouseEvent *event) {
    mouseMove(event);

    if(event->button() == Qt::LeftButton) {
        currentArea.line.push_back(currentPoint);
    } else {
        currentArea.line.push_back(currentPoint);

        state->areas.push_back(currentArea);
        currentArea.line.clear();
    }


    this->repaint();
}

void Canvas::mouseMove(QMouseEvent *event) {
    QPointF p(event->x() / scale, event->y() / scale);

    if(selection) {

        selection->setBottomRight(p);
        selection.reset();


    }

    currentPoint.p = QPointF(event->x() / scale, event->y() / scale);
    this->repaint();

}

void Canvas::mouseReleaseEvent(QMouseEvent *event) {
    mouseMove(event);

    if(selection) {
        selection.reset();
    }

}

void Canvas::mouseMoveEvent(QMouseEvent *event) {
    mouseMove(event);
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
        drawArea(a, &painter);
    }

    Area a = currentArea;

    if(selection) {
        painter.drawRect(*selection);
    } else {
        a.line.push_back(currentPoint);
    }

    drawArea(a, &painter);

}


void Canvas::setBrushWidth(int width) {
    currentWidth = width;
    repaint();
}

void Canvas::cancel() {
    currentArea.line.clear();
    repaint();
}


