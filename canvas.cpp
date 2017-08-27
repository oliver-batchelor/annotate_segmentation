#include "canvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QPolygonF>
#include <QImage>
#include <QRgb>

#include <set>

#include <iostream>

#include <opencv2/imgproc.hpp>
#include <opencv2/ximgproc.hpp>

QVector<QRgb> makeColorTable();


Canvas::Canvas()
        : defaultLabel(0), currentZoom(1.0f), labelAlpha(0.3), mode(Lines), drawing(false), spSize(10), spSmoothness(3), spOpacity(50) {
    setMouseTracking(true);
    currentPoint.r = 20.0;

    colorTable = makeColorTable();


    int fps = 20;

    timer = new QTimer();
    timer->setInterval(1000.0/fps);

    connect( timer, SIGNAL( timeout() ), this, SLOT( repaint() ) );

    timer->start();
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
            cv::scaleAdd(m, -(spOpacity / 100.0), image, m);
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

inline QVector<QRgb> make_palette(Config const &c) {
    QVector<QRgb> palette(256, c.ignore_color.rgba());

    int i = 0;
    for (Label const& l : c.labels) {
        palette[i++] = l.color.rgba();
    }

    return palette;
}


void Canvas::setConfig(Config const &c) {
    defaultLabel = c.default_label;
    colorTable = make_palette(c);
}





void Canvas::setImage(Image const &i) {

    image = i.image;
    probs = i.probs;

    overlay = cv::Mat1b();

    genOverlay();

    if(i.labels.cols == image.cols && i.labels.rows == image.rows) {
        mask = i.labels;
    } else {
        mask = cv::Mat1b(image.rows, image.cols);
        mask = defaultLabel;
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



inline void drawPoint(cv::Mat1b &image, Point const &p, int label) {
    cv::Scalar c(label, label, label);
    cv::circle(image, cv::Point(p.p.x, p.p.y), p.r, c, -1);
}

inline void drawPoly(cv::Mat1b &image, std::vector<cv::Point2f> const &points, int label) {
    cv::Scalar c(label, label, label);

    std::vector<cv::Point> ps;
    for(auto const& p : points) {
        ps.push_back(p);
    }


    std::vector<std::vector<cv::Point>> pts = {ps};
    cv::fillPoly(image, pts, c);

}

inline void drawSP(cv::Mat1b &image, cv::Mat1i const& spLabels, Point const &p, int label) {

    cv::Point c = p.p;
    int r = p.r;

    std::set<int> labels;
    for(int i = -p.r; i <= r; ++i) {
        for(int j = -p.r; j <= r; ++j) {
            if(i * i + j * j < r * r) {
                int x = c.x + j;
                int y = c.y + i;

                if(y < spLabels.rows && x < spLabels.cols && x >= 0 && y >= 0) {
                    int spLabel = spLabels(y, x);
                    labels.insert(spLabel);
                }
            }
        }
    }



    for (int l : labels) {
        cv::Mat1b mask = spLabels == l;
        image.setTo(label, mask);
    }
}



inline void floodFill(cv::Mat1b &image, Point const &p, int label) {
    cv::Scalar c(label, label, label);
    cv::floodFill(image, cv::Point(p.p.x, p.p.y), c);
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
        floodFill(mask, currentPoint, currentLabel);
    break;
    case Lines:
        if(event->button() == Qt::LeftButton) {
            if(currentLine) {
                snapshot();
                drawLine(mask, *currentLine, currentPoint, currentLabel);

                if(event->modifiers() & Qt::ControlModifier) {
                    currentLine = currentPoint;

                } else {
                    logEvent("end_drawing");
                    currentLine.reset();
                }
            } else {
                logEvent("begin_drawing");
                currentLine = currentPoint;
            }

        }
    break;
    case Points:
        snapshot();

        currentPoint.p = p;
        drawPoint(mask, currentPoint, currentLabel);

        logEvent("begin_drawing");
        drawing = true;

    break;

    case Polygons:
        if(event->button() == Qt::LeftButton) {
            if(currentPoly.empty())
                logEvent("begin drawing");


            currentPoly.push_back(p);
        } else {

            currentPoly.push_back(p);
            if(currentPoly.size() > 2) {
                logEvent("end drawing");
                snapshot();

                drawPoly(mask, currentPoly, currentLabel);
            }
            currentPoly.clear();
        }


    case SuperPixels:
        snapshot();

        currentPoint.p = p;
        drawSP(mask, spLabels, currentPoint, currentLabel);

        logEvent("begin_drawing");
        drawing = true;
    default:
    break;
    }

    //this->repaint();
}


void Canvas::deleteSelection() {
    snapshot();

    cv::Mat1b s = selectionMask();
    s = 0;

    //repaint();
}

cv::Mat1b Canvas::selectionMask() {
    if(selection) {
        return mask(*selection);
    }

    cv::Rect rect(0, 0, mask.cols, mask.rows);
    return mask(rect);
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
            drawPoint(mask, currentPoint, currentLabel);

        }
    break;



    case SuperPixels:
        if(drawing) {
            drawSP(mask, spLabels, currentPoint, currentLabel);
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
        logEvent("end_drawing");
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


    cv::Mat1b edit = mask;

    if(mode == Lines && currentLine) {
        edit = mask.clone();
        drawLine(edit, *currentLine, currentPoint, currentLabel);
    }

    QImage indexMask(edit.data, edit.cols, edit.rows, QImage::Format_Indexed8);
    indexMask.setColorTable(colorTable);

    QPixmap pm = QPixmap::fromImage(indexMask);

    painter.setRenderHint(QPainter::Antialiasing, false);

    painter.setOpacity(labelAlpha);
    //=painter.setCompositionMode(QPainter::CompositionMode_Plus);
    painter.drawPixmap(0, 0, pm);


    //painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setOpacity(1);

    QColor lc = QColor(colorTable[currentLabel]);
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

void Canvas::setMask(cv::Mat1b const& mask_) {
    if(mask_.cols == image.cols && mask_.rows == image.rows) {
        snapshot();
        mask = mask_.clone();
    }

    cancel();
}




void Canvas::setBrushWidth(int width) {
    currentPoint.r = width;

   // repaint();
}

void Canvas::cancel() {
   selection.reset();

   if(currentLine || drawing || !currentPoly.empty()) {
       logEvent("end_drawing");
   }

   currentPoly.clear();
   currentLine.reset();
   drawing = false;

   //repaint();
}



void Canvas::grabCut() {

    cv::Mat cutImage = image;

    if(int(probs.size()) > currentLabel) {

        std::vector<cv::Mat1b> channels;
        cv::split(image, channels);
        cv::Mat1b rb, gb;

        cv::addWeighted(channels[0], 0.5, channels[2], 0.5, 0.0, rb);
        cv::addWeighted(channels[1], 0.5, channels[2], 0.5, 0.0, gb);

        std::vector<cv::Mat1b> new_channels = {rb, gb, probs[currentLabel]};
        cv::merge(channels, cutImage);
    }


    cv::Mat1b m(mask.rows, mask.cols);
    m.setTo(cv::GC_PR_BGD);

    m.setTo(cv::GC_BGD,    (mask == 0));
    m.setTo(cv::GC_PR_FGD,    (mask == 1));


    cv::Mat fgModel, bgModel;
    cv::grabCut(cutImage, m, cv::Rect(), fgModel, bgModel, 2, cv::GC_INIT_WITH_MASK);

    cv::Mat1b result(mask.rows, mask.cols);
    result.setTo(0);

    result.setTo(1, (m == cv::GC_FGD) | (m == cv::GC_PR_FGD));
    setMask(result);

}



void Canvas::genOverlay() {
    using namespace cv::ximgproc;
    overlay = cv::Mat1b();
    spLabels = cv::Mat1b();

    if(!image.empty() && mode == SuperPixels) {

        int bins = 5;
        int levels = 8;
        int n = (image.rows * image.cols) / (spSize*spSize);

        std::vector<cv::Mat1b> channels;
        cv::split(image, channels);
        if(int(probs.size()) > currentLabel) {
            channels.push_back(probs[currentLabel]);
        }

        cv::Mat composite;
        cv::merge(channels, composite);

        auto sp = cv::ximgproc::createSuperpixelSEEDS(image.cols, image.rows, channels.size(), n, levels, 1 + 4 * (spSmoothness / 100.0), bins, true);
        sp->iterate(composite);


//        auto sp = createSuperpixelSLIC(image, SLICO, spSize, spSmoothness);
//        sp->iterate(10);

//        auto sp = createSuperpixelLSC(image, spSize, spSmoothness/400.0);
//        sp->iterate(4);


        sp->getLabelContourMask(overlay);
        cv::GaussianBlur(overlay, overlay, cv::Size(3, 3), 1.0, 1.0);

        sp->getLabels(spLabels);
    }

    genScaledImage();
}


void Canvas::setMode(DrawMode mode_) {
    cancel();

    mode = mode_;
    genOverlay();
}


void Canvas::undo() {

    cancel();
    if(undos.size()) {

        redos.push_back(mask);
        mask = undos.back();

        undos.pop_back();
    }

    //repaint();
}


void Canvas::redo() {
    cancel();

    if(redos.size()) {
        undos.push_back(mask);
        mask = redos.back();

        redos.pop_back();
    }

   // repaint();
}





QVector<QRgb> makeColorTable() {

    QVector<QRgb> colors = {
        0x000000, 0xFFFF00, 0x1CE6FF, 0xFF34FF, 0xFF4A46, 0x008941, 0x006FA6, 0xA30059,
        0xFFDBE5, 0x7A4900, 0x0000A6, 0x63FFAC, 0xB79762, 0x004D43, 0x8FB0FF, 0x997D87,
        0x5A0007, 0x809693, 0xFEFFE6, 0x1B4400, 0x4FC601, 0x3B5DFF, 0x4A3B53, 0xFF2F80,
        0x61615A, 0xBA0900, 0x6B7900, 0x00C2A0, 0xFFAA92, 0xFF90C9, 0xB903AA, 0xD16100,
        0xDDEFFF, 0x000035, 0x7B4F4B, 0xA1C299, 0x300018, 0x0AA6D8, 0x013349, 0x00846F,
        0x372101, 0xFFB500, 0xC2FFED, 0xA079BF, 0xCC0744, 0xC0B9B2, 0xC2FF99, 0x001E09,
        0x00489C, 0x6F0062, 0x0CBD66, 0xEEC3FF, 0x456D75, 0xB77B68, 0x7A87A1, 0x788D66,
        0x885578, 0xFAD09F, 0xFF8A9A, 0xD157A0, 0xBEC459, 0x456648, 0x0086ED, 0x886F4C,
        0x34362D, 0xB4A8BD, 0x00A6AA, 0x452C2C, 0x636375, 0xA3C8C9, 0xFF913F, 0x938A81,
        0x575329, 0x00FECF, 0xB05B6F, 0x8CD0FF, 0x3B9700, 0x04F757, 0xC8A1A1, 0x1E6E00,
        0x7900D7, 0xA77500, 0x6367A9, 0xA05837, 0x6B002C, 0x772600, 0xD790FF, 0x9B9700,
        0x549E79, 0xFFF69F, 0x201625, 0x72418F, 0xBC23FF, 0x99ADC0, 0x3A2465, 0x922329,
        0x5B4534, 0xFDE8DC, 0x404E55, 0x0089A3, 0xCB7E98, 0xA4E804, 0x324E72, 0x6A3A4C,
        0x83AB58, 0x001C1E, 0xD1F7CE, 0x004B28, 0xC8D0F6, 0xA3A489, 0x806C66, 0x222800,
        0xBF5650, 0xE83000, 0x66796D, 0xDA007C, 0xFF1A59, 0x8ADBB4, 0x1E0200, 0x5B4E51,
        0xC895C5, 0x320033, 0xFF6832, 0x66E1D3, 0xCFCDAC, 0xD0AC94, 0x7ED379, 0x012C58,
        0x7A7BFF, 0xD68E01, 0x353339, 0x78AFA1, 0xFEB2C6, 0x75797C, 0x837393, 0x943A4D,
        0xB5F4FF, 0xD2DCD5, 0x9556BD, 0x6A714A, 0x001325, 0x02525F, 0x0AA3F7, 0xE98176,
        0xDBD5DD, 0x5EBCD1, 0x3D4F44, 0x7E6405, 0x02684E, 0x962B75, 0x8D8546, 0x9695C5,
        0xE773CE, 0xD86A78, 0x3E89BE, 0xCA834E, 0x518A87, 0x5B113C, 0x55813B, 0xE704C4,
        0x00005F, 0xA97399, 0x4B8160, 0x59738A, 0xFF5DA7, 0xF7C9BF, 0x643127, 0x513A01,
        0x6B94AA, 0x51A058, 0xA45B02, 0x1D1702, 0xE20027, 0xE7AB63, 0x4C6001, 0x9C6966,
        0x64547B, 0x97979E, 0x006A66, 0x391406, 0xF4D749, 0x0045D2, 0x006C31, 0xDDB6D0,
        0x7C6571, 0x9FB2A4, 0x00D891, 0x15A08A, 0xBC65E9, 0xFFFFFE, 0xC6DC99, 0x203B3C,
        0x671190, 0x6B3A64, 0xF5E1FF, 0xFFA0F2, 0xCCAA35, 0x374527, 0x8BB400, 0x797868,
        0xC6005A, 0x3B000A, 0xC86240, 0x29607C, 0x402334, 0x7D5A44, 0xCCB87C, 0xB88183,
        0xAA5199, 0xB5D6C3, 0xA38469, 0x9F94F0, 0xA74571, 0xB894A6, 0x71BB8C, 0x00B433,
        0x789EC9, 0x6D80BA, 0x953F00, 0x5EFF03, 0xE4FFFC, 0x1BE177, 0xBCB1E5, 0x76912F,
        0x003109, 0x0060CD, 0xD20096, 0x895563, 0x29201D, 0x5B3213, 0xA76F42, 0x89412E,
        0x1A3A2A, 0x494B5A, 0xA88C85, 0xF4ABAA, 0xA3F3AB, 0x00C6C8, 0xEA8B66, 0x958A9F,
        0xBDC9D2, 0x9FA064, 0xBE4700, 0x658188, 0x83A485, 0x453C23, 0x47675D, 0x3A3F00,
        0x061203, 0xDFFB71, 0x868E7E, 0x98D058, 0x6C8F7D, 0xD7BFC2, 0x3C3E6E, 0xD83D66,
        0x2F5D9B, 0x6C5E46, 0xD25B88, 0x5B656C, 0x00B57F, 0x545C46, 0x866097, 0x365D25,
        0x252F99, 0x00CCFF, 0x674E60, 0xFC009C, 0x92896B
    };

    for(int i = 1; i < colors.size(); ++i) {
        QColor col(colors[i]);
        col.setAlpha(255);

        colors[i] = col.rgb();
    }


    return colors;
}
