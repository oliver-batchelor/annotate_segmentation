#ifndef CANVAS_H
#define CANVAS_H


#include <QWidget>
#include "state.h"

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "opencv2/core.hpp"

enum DrawMode {
    Selection,
    Lines,
    Points
};


class Canvas : public QWidget
{
    Q_OBJECT

public:
    Canvas();

    void setImage(QPixmap const& p,  cv::Mat1b mask = cv::Mat1b());
    void setMask(cv::Mat1b const& mask);

    cv::Mat1b getMask() const { return mask; }

    bool isModified() { return undos.size() || redos.size(); }


public slots:
    void zoom(float zoom);

    void setLabel(int label);
    void setBrushWidth(int width);

    void setMode(DrawMode mode);

    void setSelect() { setMode(Selection); }\
    void setPoints() { setMode(Points); }
    void setLines() { setMode(Lines); }

    void undo();
    void redo();
    void cancel();

    void deleteSelection();


    void snapshot() {
        undos.push_back(mask.clone());
        redos.clear();
    }

protected:

    cv::Mat1b selectionMask();


    void setPoint(QPointF const &p);

    //    bool event(QEvent *event);
    //void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);



private:
    void mouseMove(QMouseEvent *event);

    boost::optional<Point> currentLine;
    Point currentPoint;

    int currentLabel;

    cv::Mat1b mask;

    float scale;
    DrawMode mode;

    bool drawing;

    boost::optional<cv::Rect2f> selection;
    boost::optional<cv::Point2f> selecting;

    QPixmap image;
    QPixmap scaled;

    QVector<QRgb> colorTable;

    std::vector<cv::Mat1b> undos;
    std::vector<cv::Mat1b> redos;
};

#endif // CANVAS_H
