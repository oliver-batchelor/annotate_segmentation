#ifndef STATE_H
#define STATE_H

#include <memory>
#include <QPoint>
#include <QLine>
#include <QString>

#include "opencv2/core.hpp"

struct Point {
    Point(cv::Point2f const &p, float r)
        : p(p), r(r) {}
    Point() : r(1.0f) {}

    cv::Point2f p;
    float r;
};


struct Label {
    Label(std::string name, int value) :
        name(name), value(value) {}

    Label() : value(0) {}

    std::string name;
    int value;

};


class Config {
public:

    std::vector<Label> labels;
};

#endif // STATE_H
