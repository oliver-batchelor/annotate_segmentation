#ifndef LAYER_H
#define LAYER_H

#include <QWidget>
#include <QTimer>
#include <QTime>

#include <QImage>
#include <QRgb>

#include "opencv2/core.hpp"
#include <opencv2/imgproc.hpp>
#include "state.h"

QVector<QRgb> makeColorTable();


class Layer {

public:

    Layer(int default_label=0) :
       dirty(false), default_label(default_label), opacity(30)
    {
        palette = makeColorTable();
    }

    void setDefaultLabel(int label) {
        default_label = label;
    }


    void setMask(cv::Mat1b const& indices) {
        image = indices;
    }

    cv::Mat1b const &getMask() const {
        return image;
    }

    void reset(int rows, int cols) {
        image = cv::Mat1b(rows, cols);
        image = default_label;

        dirty = true;
    }


    void setPalette(QVector<QRgb> const &palette_) {
        palette = palette_;
        dirty = true;

    }

    QPixmap const& getPixmap();

    QColor getColor(int label) {
        label = std::min<int>(label, palette.size());
        return QColor(palette[label]);
    }


    void drawPoint(Point const &p, int label);
    void drawPoly(std::vector<cv::Point2f> const &points, int label);
    void drawSP(cv::Mat1i const& spLabels, Point const &p, int label);

    void drawLine(Point const &start, Point const& end, int label);

    void floodFill(Point const &p, int label);
    void drawRect(cv::Rect2f const &s, int label);

    void clearRect(cv::Rect2f const &s) {
        drawRect(s, default_label);
    }

    void setOpacity(int opacity_) {
        opacity = opacity_;
    }

    int getOpacity() const { return opacity; }

private:

    QPixmap pixmap;
    cv::Mat1b image;

    QVector<QRgb> palette;
    bool dirty;

    int default_label;
    int opacity;

};


typedef std::shared_ptr<Layer> LayerPtr;

#endif // LAYER_H
