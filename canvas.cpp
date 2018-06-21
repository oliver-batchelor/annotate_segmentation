#include "canvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QPolygonF>
#include <QImage>
#include <QRgb>

#include <set>

#include <iostream>

#include <opencv2/imgproc.hpp>


QVector<QRgb> makeColorTable();


Canvas::Canvas()
        : defaultLabel(0), currentZoom(1.0f), mode(Lines), drawing(false), overlayOpacity(50) {
    setMouseTracking(true);
    currentPoint.r = 20.0;

    int fps = 20;

    timer = new QTimer();
    timer->setInterval(1000.0/fps);

    connect( timer, SIGNAL( timeout() ), this, SLOT( repaint() ) );

    timer->start();
}




void Canvas::zoomIn() {
    zoom(std::min<float>(4, currentZoom / 0.95));
}

void Canvas::zoomOut() {
    zoom(std::max<float>(0.25, currentZoom * 0.95));
}


void Canvas::genScaledImage() {

    if(!image.empty()) {

        cv::Mat3b m = image.clone();

        if(!overlay.empty()) {
            cv::cvtColor(overlay, m, cv::COLOR_GRAY2BGR);
            cv::scaleAdd(m, -(overlayOpacity / 100.0), image, m);
        }

        cv::Mat3b s(image.rows * currentZoom, image.cols * currentZoom);

        int interp = currentZoom < 1 ? cv::INTER_AREA : cv::INTER_CUBIC;
        cv::resize(m, s, s.size(), currentZoom, currentZoom, interp);

        QImage i(s.data, s.cols, s.rows, s.step, QImage::Format_RGB888);
        scaled = QPixmap::fromImage(i.copy());

    } else {
        scaled = QPixmap();
    }
}


void Canvas::zoom(float level) {

    currentZoom = level;

    genScaledImage();
    resize(scaled.size());
}



void Canvas::setImage(cv::Mat3b const &image_) {

    cancel();

    overlay = cv::Mat1b();
    spLabels = cv::Mat1b();
    image = image_;

    for (auto const& l : layers) {
        l->reset(image.rows, image.cols);
    }


    selecting.reset();
    selection.reset();

    undos.clear();
    redos.clear();

    zoom(currentZoom);
    resetLog();
}

void Canvas::setLabel(int label) {
    currentLabel = label;
    repaint();
}



inline cv::Point2f Canvas::getPosition(QMouseEvent *event) {

    cv::Point2f p(event->x() / currentZoom, event->y() / currentZoom);
    p.x = std::min<float>(p.x, image.cols - 1);
    p.y = std::min<float>(p.y, image.rows - 1);

    p.x = std::max<float>(p.x, 0.0f);
    p.y = std::max<float>(p.y, 0.0f);

    return p;
}

void Canvas::mousePressEvent(QMouseEvent *event) {
    cv::Point2f p = getPosition(event);
    logEvent("click");

    mouseMove(event);

    switch(mode) {
    case Selection:
        selection = cv::Rect2f(p, p);
        selecting = p;

    break;
    case Fill:
        snapshot();
        activeLayer->floodFill(currentPoint, currentLabel);
    break;
    case Lines:
        if(event->button() == Qt::LeftButton) {
            if(currentLine) {
                snapshot();
                activeLayer->drawLine(*currentLine, currentPoint, currentLabel);

                if(event->modifiers() & Qt::ControlModifier) {
                    currentLine = currentPoint;

                } else {
                    logEvent("end lines");
                    currentLine.reset();
                }
            } else {
                logEvent("begin lines");
                currentLine = currentPoint;
            }

        }
    break;
    case Points:
        snapshot();

        currentPoint.p = p;
        activeLayer->drawPoint(currentPoint, currentLabel);

        logEvent("begin points");
        drawing = true;

    break;

    case Polygons:
        if(event->button() == Qt::LeftButton) {
            if(currentPoly.empty())
                logEvent("begin polygon");


            currentPoly.push_back(p);
        } else {

            currentPoly.push_back(p);
            if(currentPoly.size() > 2) {
                logEvent("end polygon");
                snapshot();

                activeLayer->drawPoly(currentPoly, currentLabel);
            }
            currentPoly.clear();
        }
    break;


    case SuperPixels:
        snapshot();

        currentPoint.p = p;
        activeLayer->drawSP(spLabels, currentPoint, currentLabel);

        logEvent("begin superpixels");
        drawing = true;
    default:
    break;
    }

    //this->repaint();
}

cv::Rect2f Canvas::getSelection() {
    if(selection) {
        return *selection;
    } else {
        return cv::Rect2f(0, 0, image.cols, image.rows);
    }
}

void Canvas::deleteSelection() {
    snapshot();

    activeLayer->clearRect(getSelection());
}



void Canvas::mouseMove(QMouseEvent *event) {

    cv::Point2f p = getPosition(event);

    if(selecting) {

        cv::Point2f topLeft (std::min<float>(selecting->x, p.x), std::min<float>(selecting->y, p.y));
        cv::Point2f bottomRight (std::max<float>(selecting->x, p.x), std::max<float>(selecting->y, p.y));

        selection = cv::Rect2f(topLeft, bottomRight);

    } else {

        if(event->modifiers() & Qt::ShiftModifier) {
            cv::Point2f d = p - currentPoint.p;
            currentPoint.r = std::min<float>(200, std::max<float>(1, currentPoint.r - d.y));

            brushWidthChanged(currentPoint.r);
        }
    }

    currentPoint.p = p;

    switch(mode) {
    case Points:
        if(drawing) {
            activeLayer->drawPoint(currentPoint, currentLabel);

        }
    break;



    case SuperPixels:
        if(drawing) {
            activeLayer->drawSP(spLabels, currentPoint, currentLabel);
        }
    break;

    default: break;
    }


    //this->repaint();
}


void Canvas::mouseReleaseEvent(QMouseEvent *event) {
    mouseMove(event);

    if(selecting) {

        if(selection->area() > 0) {
            logEvent("selection");
        }

        selecting.reset();
    }

    if(drawing) {
        logEvent("end points");
    }

    drawing = false;
    //repaint();
}

void Canvas::mouseMoveEvent(QMouseEvent *event) {
    mouseMove(event);
}




void Canvas::paintEvent(QPaintEvent * /* event */) {
    QPainter painter(this);

    painter.fillRect(rect(), QColor(Qt::gray));
    painter.drawPixmap(0, 0, scaled);

    painter.scale(currentZoom, currentZoom);

    QPen pen;
    pen.setWidth(2);
    painter.setPen(pen);


    QPixmap pm;
    cv::Mat1b edit;

    if(mode == Lines && currentLine) {
        edit = activeLayer->getMask().clone();
        activeLayer->drawLine(*currentLine, currentPoint, currentLabel);
    }


    painter.setRenderHint(QPainter::Antialiasing, false);

    for (auto const& layer : layers) {
        painter.setOpacity(float(layer->getOpacity()) / 100.0f);
        painter.drawPixmap(0, 0,  layer->getPixmap());
    }


    if(mode == Lines && currentLine) {
        activeLayer->setMask(edit);
    }

    painter.setOpacity(1);

    QColor lc = QColor(activeLayer->getColor(currentLabel));
    lc.setAlpha(80);

    painter.setRenderHint(QPainter::Antialiasing, true);


    switch(mode) {


    case Selection:
        if(selection) {

            painter.setBrush(QBrush(QColor(0, 127, 127, 127)));

            QRectF rect(selection->x, selection->y, selection->width, selection->height);
            painter.drawRect(rect);
        }
    break;
    case Points:
    case SuperPixels:
        painter.setBrush(QBrush(lc));
        painter.drawEllipse(QPointF(currentPoint.p.x, currentPoint.p.y), currentPoint.r, currentPoint.r);
    break;
    case Lines:

        painter.setBrush(QBrush(lc));
        painter.drawEllipse(QPointF(currentPoint.p.x, currentPoint.p.y), currentPoint.r, currentPoint.r);
    break;

    case Polygons: {
        painter.setBrush(QBrush(lc));
        std::vector<QPointF> points;
        for(auto const& p : currentPoly) {
            points.push_back(QPointF(p.x, p.y));
        }

        points.push_back(QPointF(currentPoint.p.x, currentPoint.p.y));
        painter.drawPolygon(&points.front(), currentPoly.size() + 1);
    }
    break;


    default: break;
    }

}




void Canvas::setBrushWidth(int width) {
    currentPoint.r = width;

   // repaint();
}

void Canvas::cancel() {
   selection.reset();

   if(currentLine) logEvent("end lines");
   else if(drawing) logEvent("end points");
   else if(!currentPoly.empty()) logEvent("end polygons");


   currentPoly.clear();
   currentLine.reset();
   drawing = false;

   //repaint();
}



//void Canvas::grabCut() {

//    cv::Mat cutImage = image;

//    if(int(probs.size()) > currentLabel) {

//        std::vector<cv::Mat1b> channels;
//        cv::split(image, channels);
//        cv::Mat1b rb, gb;

//        cv::addWeighted(channels[0], 0.5, channels[2], 0.5, 0.0, rb);
//        cv::addWeighted(channels[1], 0.5, channels[2], 0.5, 0.0, gb);

//        std::vector<cv::Mat1b> new_channels = {rb, gb, probs[currentLabel]};
//        cv::merge(channels, cutImage);
//    }


//    cv::Mat1b m(mask.rows, mask.cols);
//    m.setTo(cv::GC_PR_BGD);

//    m.setTo(cv::GC_BGD,    (mask == 0));
//    m.setTo(cv::GC_PR_FGD,    (mask == 1));


//    cv::Mat fgModel, bgModel;
//    cv::grabCut(cutImage, m, cv::Rect(), fgModel, bgModel, 2, cv::GC_INIT_WITH_MASK);

//    cv::Mat1b result(mask.rows, mask.cols);
//    result.setTo(0);

//    result.setTo(1, (m == cv::GC_FGD) | (m == cv::GC_PR_FGD));
//    setMask(result);

//}



//void Canvas::genOverlay() {
//    using namespace cv::ximgproc;
//    overlay = cv::Mat1b();
//    spLabels = cv::Mat1b();

//    if(!image.empty() && mode == SuperPixels) {

//        int bins = 5;
//        int levels = 8;
//        int n = (image.rows * image.cols) / (spSize*spSize);

//        std::vector<cv::Mat1b> channels;
//        cv::split(image, channels);
//        if(int(probs.size()) > currentLabel) {
//            channels.push_back(probs[currentLabel]);
//        }

//        cv::Mat composite;
//        cv::merge(channels, composite);

//        auto sp = cv::ximgproc::createSuperpixelSEEDS(image.cols, image.rows, channels.size(), n, levels, 1 + 4 * (spSmoothness / 100.0), bins, true);
//        sp->iterate(composite);

//        sp->getLabelContourMask(overlay);
//        cv::GaussianBlur(overlay, overlay, cv::Size(3, 3), 1.0, 1.0);

//        sp->getLabels(spLabels);
//    }

//    genScaledImage();
//}


void Canvas::setMode(DrawMode mode_) {
    cancel();
    mode = mode_;
}


void Canvas::undo() {

    cancel();
    if(undos.size()) {

        redos.push_back(getState());
        setState(undos.back());

        undos.pop_back();
    }

    //repaint();
}


void Canvas::redo() {
    cancel();

    if(redos.size()) {
        undos.push_back(getState());
        setState(redos.back());

        redos.pop_back();
    }

   // repaint();
}






