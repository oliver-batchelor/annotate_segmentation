#ifndef STATE_H
#define STATE_H

#include <memory>
#include <QPoint>
#include <QLine>
#include <QString>



struct Point {
    Point(QPointF const &p, float r)
        : p(p), r(r) {}
    Point() : r(1.0f) {}

    QPointF p;
    float r;
};

struct Area {
    Area(int label = 0) : label(label) {}

    std::vector<Point> line;
    int label;
};

class State
{
public:
    State() {}


    QString filename;
    std::vector<Area> areas;
};





class Config {
public:

    std::vector<std::string> labels;
};

#endif // STATE_H
