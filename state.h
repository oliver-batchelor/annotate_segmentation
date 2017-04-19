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




class Config {
public:

    std::vector<std::string> labels;
};

#endif // STATE_H
