#ifndef CANVAS_H
#define CANVAS_H


#include <QWidget>
#include <QTimer>
#include <QTime>
#include "state.h"

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "opencv2/core.hpp"

enum DrawMode {
    Selection,
    Lines,
    Points,
    Fill,
    SuperPixels,
    Polygons
};


struct Image {

    cv::Mat3b image;
    cv::Mat1b labels;
    cv::Mat1b prediction;

    std::vector<cv::Mat1b> probs;
};

struct Event {

    float time;
    std::string event;
};

class Canvas : public QWidget
{
    Q_OBJECT

public:
    Canvas();

    void setConfig(Config const &c);

    void setImage(Image const &image);
    void setMask(cv::Mat1b const& mask);


    cv::Mat3b const& getImage() const { return image; }
    cv::Mat1b const& getMask() const { return mask; }

    bool isModified() { return undos.size() || redos.size(); }


    void resetLog() {
        log.clear();
        time.start();
    }

    void logEvent(std::string const &event) {

        Event e;
        e.time = float(time.elapsed()) / 1000.0f;
        e.event = event;

        log.push_back(e);
    }

    std::vector<Event> getLog() {
        return log;
    }

public slots:
    void zoomIn();
    void zoomOut();

    void zoom(float zoom);

    void setLabel(int label);
    void setBrushWidth(int width);

    void setMode(DrawMode mode);
    void grabCut();


    void setSelect() { setMode(Selection); }\
    void setPoints() { setMode(Points); }
    void setLines() { setMode(Lines); }
    void setFill() { setMode(Fill); }

    void setSuperPixels(cv::Mat1b const &spLabels, cv::Mat1b const& overlay) {
        overlay = boundaries;
        spImage = spLabels;

        setMode(SuperPixels);
    }


    void setPolygons() { setMode(Polygons); }


    void setSPOpacity(int n) {
        spOpacity = n;
        genScaledImage();
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

    cv::Point2f getPosition(QMouseEvent *event);



private:
    void mouseMove(QMouseEvent *event);


    boost::optional<Point> currentLine;
    std::vector<cv::Point2f> currentPoly;

    Point currentPoint;

    int defaultLabel;


    int currentLabel;
    float currentZoom;

    float labelAlpha;

    cv::Mat1b mask;

    DrawMode mode;
    bool drawing;

    boost::optional<cv::Rect2f> selection;
    boost::optional<cv::Point2f> selecting;

    std::vector<cv::Mat1b> probs;
    cv::Mat3b image;

    cv::Mat1b overlay;
    cv::Mat1i spLabels;

    QPixmap scaled;

    QVector<QRgb> colorTable;

    std::vector<cv::Mat1b> undos;
    std::vector<cv::Mat1b> redos;


    int spSize;
    int spSmoothness;
    int spOpacity;

    QTimer *timer;
    QTime time;

    std::vector<Event> log;


};

#endif // CANVAS_H
