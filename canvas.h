#ifndef CANVAS_H
#define CANVAS_H


#include <QWidget>
#include <QTimer>
#include "state.h"

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "opencv2/core.hpp"

enum DrawMode {
    Selection,
    Lines,
    Points,
    Fill,
    SuperPixels
};


class Canvas : public QWidget
{
    Q_OBJECT

public:
    Canvas();

    void setImage(cv::Mat3b const &image, cv::Mat1b const &mask = cv::Mat1b());
    void setMask(cv::Mat1b const& mask);

    cv::Mat1b getMask() const { return mask; }

    bool isModified() { return undos.size() || redos.size(); }


public slots:
    void zoomIn();
    void zoomOut();

    void zoom(float zoom);

    void setLabel(int label);
    void setBrushWidth(int width);

    void setMode(DrawMode mode);

    void setSelect() { setMode(Selection); }\
    void setPoints() { setMode(Points); }
    void setLines() { setMode(Lines); }
    void setFill() { setMode(Fill); }
    void setSuperPixels() { setMode(SuperPixels); }

    void setSPSize(int n) {
        spSize = n;
        genOverlay();
    }

    void setSPSmoothness(int n) {
        spSmoothness = n;
        genOverlay();
    }

    void undo();
    void redo();
    void cancel();

    void deleteSelection();


    void snapshot() {
        undos.push_back(mask.clone());
        redos.clear();
    }

signals:

    void brushWidthChanged(int size);

protected:

    cv::Mat1b selectionMask();


    void setPoint(QPointF const &p);

    //    bool event(QEvent *event);
    //void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

    void genScaledImage();
    void genOverlay();


private:
    void mouseMove(QMouseEvent *event);


    boost::optional<Point> currentLine;
    Point currentPoint;

    int currentLabel;
    float currentZoom;

    cv::Mat1b mask;

    DrawMode mode;
    bool drawing;

    boost::optional<cv::Rect2f> selection;
    boost::optional<cv::Point2f> selecting;

    cv::Mat3b image;

    cv::Mat1b overlay;
    cv::Mat1i spLabels;

    QPixmap scaled;

    QVector<QRgb> colorTable;

    std::vector<cv::Mat1b> undos;
    std::vector<cv::Mat1b> redos;


    int spSize;
    int spSmoothness;

    QTimer *timer;


};

#endif // CANVAS_H
