#include "canvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QPolygonF>

#include <iostream>

Canvas::Canvas(std::shared_ptr<State> state)
        : state(state), scale(1.0f) {
    setMouseTracking(true);
    currentPoint.r = 20.0;
}

float length(QPointF const &p) {
    return std::sqrt(QPointF::dotProduct(p, p));
}


inline QPointF perpLine(QPointF const &start, QPointF const &end) {
    QPointF line = end - start;
    QPointF dir = line / length(line);

    return QPointF (dir.y(), -dir.x());
}

QPolygonF circle(float r) {

    const double pi = 3.1415926535897;
    float c = r * 2 * pi;

    size_t sides = std::max<size_t>(4, size_t(c / 4.0));
    float inc = (2 * pi) / float(sides);

    QVector<QPointF> v;
    for(size_t i = 0; i < sides; ++i) {
        float t = inc * i;
        v.push_back(QPointF(r * std::sin(t), r * cos(t)));
    }

    return QPolygonF (v);
}

QPolygonF point(Point const &p) {
    QPolygonF c = circle(p.r);
    c.translate(p.p);

    return c;
}

inline QPolygonF makeRect(Point const &p1, Point const p2) {
    QPointF perp = perpLine(p1.p, p2.p);

    QVector<QPointF> v = {p1.p + perp * p1.r, p2.p + perp * p2.r, p2.p - perp * p2.r, p1.p - perp * p1.r};
    return QPolygonF(v);
}


inline QPolygonF areaPoly(Area const& a) {
    QPolygonF poly = point(a.line[0]);
    for(size_t i = 1; i < a.line.size(); ++i) {
        if(length(a.line[i - 1].p - a.line[i].p) > 0.01) {

            QPolygonF strip = makeRect(a.line[i - 1], a.line[i]);
            poly = poly.united(strip).united(point(a.line[i]));
        }
    }

    return poly;

}

inline void drawAreaMask(Area const &a, QPainter *painter) {

    QColor c (a.label + 1, a.label + 1, a.label + 1);
    painter->setBrush(c);
    painter->setPen(Qt::NoPen);

    QPolygonF poly = areaPoly(a);
    painter->drawPolygon(poly, Qt::OddEvenFill);
}


inline void drawArea(Area const &a, QPainter *painter) {

    QColor c ((Qt::GlobalColor)(a.label + 8));
    c.setAlpha(127);
    painter->setBrush(c);

    QPolygonF poly = areaPoly(a);
    painter->drawPolygon(poly, Qt::OddEvenFill);
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
        run(drawCmd(currentPoint));
    } else {
        run(endCmd(currentPoint));
    }


    this->repaint();
}

void Canvas::mouseMove(QMouseEvent *event) {
    QPointF p(event->x() / scale, event->y() / scale);

    if(selection) {

        selection->setBottomRight(p);
        selection.reset();


    }

    if(event->modifiers() & Qt::ShiftModifier) {
        currentPoint.r = length(p - currentPoint.p);
    } else {
        currentPoint.p = p;
    }

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

    painter.scale(scale, scale);
    painter.setRenderHint(QPainter::Antialiasing, true);

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



QImage Canvas::save() {
    QImage img(image.size(), QImage::Format_RGB32);
    img.fill(QColor(255, 255, 255));

    QPainter painter;
    painter.begin(&img);

    for(auto a : state->areas) {
        drawAreaMask(a, &painter);
    }

    painter.end();

    return img;
}

void Canvas::setBrushWidth(int width) {
    currentPoint.r = width;
    repaint();
}




void Canvas::undo() {
    if(undos.size()) {
        Command c = undos.back();

        undos.pop_back();
        redos.push_back(c);
        undoCmd(c);

    }
}


void Canvas::redo() {
    if(redos.size()) {
        Command c = redos.back();

        redos.pop_back();
        undos.push_back(c);

        applyCmd(c);
    }
}


void Canvas::deleteSelection() {

}


void Canvas::applyCmd(Command const& c) {
    if(Draw const *d = boost::get<Draw>(&c)) {
        currentArea.line.push_back(d->p);

    } else if(End const *e = boost::get<End>(&c)) {
        currentArea.line.push_back(e->p);

        state->areas.push_back(currentArea);
        currentArea.line.clear();

    }

    repaint();
}


void Canvas::undoCmd(Command const& c) {
    if(boost::get<Draw>(&c)) {
        currentArea.line.pop_back();

    } else if(boost::get<End>(&c)) {

        currentArea = state->areas.back();
        currentArea.line.pop_back();

        state->areas.pop_back();
    }

    repaint();
}




void Canvas::run(Command const &c) {
    redos.clear();
    applyCmd(c);

    undos.push_back(c);

}



