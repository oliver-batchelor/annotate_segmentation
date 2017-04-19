#include "canvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QPolygonF>
#include <QImage>
#include <QRgb>

#include <iostream>

#include <opencv2/imgproc.hpp>

Canvas::Canvas()
        : scale(1.0f), mode(Lines), drawing(false) {
    setMouseTracking(true);
    currentPoint.r = 20.0;

    colorTable = QVector<QRgb>(255, 0);

    for(int i = Qt::red; i < Qt::darkYellow; ++i) {
        colorTable[i - Qt::red + 1] = QColor(Qt::GlobalColor(i)).rgb();
    }

    QColor c(colorTable[0]);
}

float length(cv::Point2f const &p) {
    return std::sqrt(p.dot(p));
}


inline cv::Point2f perpLine(cv::Point2f const &start, cv::Point2f const &end) {
    cv::Point2f line = end - start;
    cv::Point2f dir = line / length(line);

    return cv::Point2f (dir.y, -dir.x);
}


inline std::vector<cv::Point2f> makeRect(Point const &p1, Point const p2) {
    cv::Point2f perp = perpLine(p1.p, p2.p);
    return std::vector<cv::Point2f> {p1.p + perp * p1.r, p2.p + perp * p2.r, p2.p - perp * p2.r, p1.p - perp * p1.r};
}




void Canvas::zoom(float level) {
    scale = level / 100;

    if(!image.isNull()) {
        scaled = image.scaled(image.size() * scale);
    } else {
        scaled = QPixmap();
    }

    resize(scaled.size());
    repaint();


}

void Canvas::setImage(QPixmap const &p, cv::Mat1b mask_) {

    image = p;
    if(mask_.cols == p.width() && mask_.rows == p.height()) {
        mask = mask_;
    } else {
        mask = cv::Mat1b(p.height(), p.width());
        mask = 0;
    }

    selecting.reset();
    selection.reset();

    undos.clear();
    redos.clear();

    zoom(scale * 100);
}

void Canvas::setLabel(int label) {
    currentLabel = label;
    repaint();
}



inline void drawPoint(cv::Mat1b &image, Point const &p, int label) {
    cv::Scalar c(label, label, label);
    cv::circle(image, cv::Point(p.p.x, p.p.y), p.r, c, -1);
}

inline void drawRect(cv::Mat1b &image, cv::Rect2f const &s, int label) {
    image(s) = label;
}

inline void drawLine(cv::Mat1b &image, Point const &start, Point const& end, int label) {
    std::vector<cv::Point2f> rect = makeRect(start, end);
    std::vector<cv::Point> points(rect.begin(), rect.end());

    cv::Scalar c(label, label, label);
    cv::fillConvexPoly(image, points, c);

    drawPoint(image, start, label);
    drawPoint(image, end, label);
}



void Canvas::mousePressEvent(QMouseEvent *event) {
    cv::Point2f p(event->x() / scale, event->y() / scale);

    mouseMove(event);

    switch(mode) {
    case Selection:
        selection = cv::Rect2f(p, p);
        selecting = p;
    break;
    case Lines:
        if(event->button() == Qt::LeftButton) {
            if(currentLine) {
                snapshot();
                drawLine(mask, *currentLine, currentPoint, currentLabel);

                if(event->modifiers() & Qt::ControlModifier) {
                    currentLine = currentPoint;

                } else {
                    currentLine.reset();
                }
            } else {
                currentLine = currentPoint;
            }

        }
    break;
    case Points:
        snapshot();

        currentPoint.p = p;
        drawPoint(mask, currentPoint, currentLabel);

        drawing = true;

    break;
    }

    this->repaint();
}


void Canvas::deleteSelection() {
    snapshot();

    cv::Mat1b s = selectionMask();
    s = 0;

    repaint();
}

cv::Mat1b Canvas::selectionMask() {
    if(selection) {
        return mask(*selection);
    }

    cv::Rect rect(0, 0, mask.cols, mask.rows);
    return mask(rect);
}


void Canvas::mouseMove(QMouseEvent *event) {
    cv::Point2f p(event->x() / scale, event->y() / scale);

    if(selecting) {

        cv::Point2f topLeft (std::min<float>(selecting->x, p.x), std::min<float>(selecting->y, p.y));
        cv::Point2f bottomRight (std::max<float>(selecting->x, p.x), std::max<float>(selecting->y, p.y));

        selection = cv::Rect2f(topLeft, bottomRight);

    } else {

        if(event->modifiers() & Qt::ShiftModifier) {
            cv::Point2f d = p - currentPoint.p;
            currentPoint.r = std::min<float>(500, std::max<float>(4, currentPoint.r - d.y));

        }
    }

    currentPoint.p = p;

    switch(mode) {
    case Points:
        if(drawing) {
            drawPoint(mask, currentPoint, currentLabel);

        }
    break;
    default: break;
    }


    this->repaint();
}


void Canvas::mouseReleaseEvent(QMouseEvent *event) {
    mouseMove(event);

    if(selecting) {
        selecting.reset();
    }

    drawing = false;
    repaint();
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

    QPen pen;
    pen.setWidth(2);
    painter.setPen(pen);


    cv::Mat1b edit = mask;

    if(mode == Lines && currentLine) {
        edit = mask.clone();
        drawLine(edit, *currentLine, currentPoint, currentLabel);
    }

    QImage indexMask(edit.data, edit.cols, edit.rows, QImage::Format_Indexed8);
    indexMask.setColorTable(colorTable);

    QPixmap pm = QPixmap::fromImage(indexMask);

    painter.setOpacity(0.4);
    painter.drawPixmap(0, 0, pm);

    painter.setOpacity(1);

    QColor lc = QColor(colorTable[currentLabel]);
    lc.setAlpha(80);

    switch(mode) {


    case Selection:
        if(selection) {

            painter.setBrush(QBrush(QColor(0, 127, 127, 127)));

            QRectF rect(selection->x, selection->y, selection->width, selection->height);
            painter.drawRect(rect);
        }
    break;
    case Points:
        painter.setBrush(QBrush(lc));
        painter.drawEllipse(QPointF(currentPoint.p.x, currentPoint.p.y), currentPoint.r, currentPoint.r);
    break;
    case Lines:

        painter.setBrush(QBrush(lc));
        painter.drawEllipse(QPointF(currentPoint.p.x, currentPoint.p.y), currentPoint.r, currentPoint.r);


    break;

    }

}

void Canvas::setMask(cv::Mat1b const& mask_) {
    mask = mask_.clone();

    undos.clear();
    redos.clear();

    currentLine.reset();

    repaint();
}


cv::Mat1b Canvas::save() {
    return mask;
}

void Canvas::setBrushWidth(int width) {
    currentPoint.r = width;
    repaint();
}

void Canvas::cancel() {
   selection.reset();
   currentLine.reset();

   drawing = false;

   repaint();
}

void Canvas::setMode(DrawMode mode_) {
    cancel();
    mode = mode_;
}


void Canvas::undo() {

    cancel();
    if(undos.size()) {

        redos.push_back(mask);
        mask = undos.back();

        undos.pop_back();
    }

    repaint();
}


void Canvas::redo() {
    cancel();

    if(redos.size()) {
        undos.push_back(mask);
        mask = redos.back();

        redos.pop_back();
    }

    repaint();
}




