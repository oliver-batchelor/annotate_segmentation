#ifndef STATE_H
#define STATE_H

#include <memory>
#include <QPoint>
#include <QLine>
#include <QString>
#include <QColor>

#include "opencv2/core.hpp"

struct Point {
    Point(cv::Point2f const &p, float r)
        : p(p), r(r) {}
    Point() : r(1.0f) {}

    cv::Point2f p;
    float r;
};


struct Label {
    Label(std::string name, int value, QColor color) :
        name(name), value(value), color(color) {}

    Label() : value(0) {}

    std::string name;
    int value;

    QColor color;

};


class Config {
public:
    Config() : default_label(0), ignore_label(255) {}

    std::vector<Label> labels;

    int default_label;

    QColor ignore_color;
    int ignore_label;


};

#endif // STATE_H
