#ifndef CANVAS_H
#define CANVAS_H


#include <QWidget>
#include <QTimer>
#include <QTime>
#include "state.h"

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "layer.h"

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



//void setConfig(Config const &c);
    void setImage(cv::Mat3b const &image);

    cv::Mat3b const& getImage() const { return image; }

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


    void setSelect() { setMode(Selection); }\
    void setPoints() { setMode(Points); }
    void setLines() { setMode(Lines); }
    void setFill() { setMode(Fill); }

    void setSuperPixels(cv::Mat1b const &spLabels_, cv::Mat1b const& overlay_) {
        overlay = overlay_;
        spLabels = spLabels_;

        setMode(SuperPixels);
    }


    void setPolygons() { setMode(Polygons); }


    void setOverlayOpacity(int n) {
        overlayOpacity = n;
        genScaledImage();
    }

    LayerPtr getLayer(int i) {
        return layers[i];
    }

    LayerPtr getActiveLayer() {
        return activeLayer;
    }

    void setLayers(std::vector<LayerPtr> const &layers_, int active=0) {
        layers = layers_;
        activeLayer = layers[active];
    }

    void setActiveLayer(int i) {
        activeLayer = layers[i];
        cancel();
    }


    void undo();
    void redo();
    void cancel();

    void deleteSelection();
    cv::Rect2f getSelection();




    void snapshot() {
        undos.push_back(getState());
        redos.clear();
    }

signals:

    void brushWidthChanged(int size);

protected:



    void setPoint(QPointF const &p);

    //    bool event(QEvent *event);
    //void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

    void genScaledImage();
    cv::Point2f getPosition(QMouseEvent *event);


    typedef std::vector<cv::Mat1b> State;

    State getState() {
        std::vector<cv::Mat1b> masks;

        for(auto const& l : layers) {
            masks.push_back(l->getMask().clone());
        }
        return masks;
    }

    void setState(State const& state) {

        for(size_t i = 0; i < state.size(); ++i) {
            layers[i]->setMask(state[i]);
        }
    }


private:
    void mouseMove(QMouseEvent *event);


    boost::optional<Point> currentLine;
    std::vector<cv::Point2f> currentPoly;

    Point currentPoint;

    int defaultLabel;

    int currentLabel;
    float currentZoom;

    DrawMode mode;
    bool drawing;

    boost::optional<cv::Rect2f> selection;
    boost::optional<cv::Point2f> selecting;

    cv::Mat3b image;
    cv::Mat1i spLabels;

    int overlayOpacity;
    cv::Mat1b overlay;

    QPixmap scaled;

    std::vector<LayerPtr> layers;

    LayerPtr activeLayer;


    std::vector<State> undos;
    std::vector<State> redos;

    QTimer *timer;
    QTime time;

    std::vector<Event> log;

};

#endif // CANVAS_H
