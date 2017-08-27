#ifndef LAYER_H
#define LAYER_H

#include <QWidget>
#include <QTimer>
#include <QTime>

#include <QImage>
#include <QRgb>

#include "opencv2/core.hpp"



class Layer {

public:

    Layer(int rows, int cols, std::vector<QRgb> const &palette) :
        palette(palette), dirty(false), image(rows, cols)
    { }


    void setMask(cv::Mat1b const& indices) {
        return image;
    }

    cv::Mat1b const &getMask() const {
        return image;
    }


    QPixmap const& getPixmap();


    void drawPoint(Point const &p, int label);
    void drawPoly(std::vector<cv::Point2f> const &points, int label);
    void drawSP(cv::Mat1i const& spLabels, Point const &p, int label);

    void floodFill(Point const &p, int label);
    void drawRect(cv::Rect2f const &s, int label);

private:

    QPixmap pixmap;
    cv::Mat1b image;

    std::vector<QRgb> palette;
    bool dirty;

};

#endif // LAYER_H
