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
    float radius;
};

struct Line {
    Line(QPointF const &start, float end, int label)
        : start(start), end(end), label (label) {}

    Line() : label(0) {}


    Point start;
    Point end;

    int label;
};



class State
{
public:
    State() {}


    QString filename;
    std::vector<std::vector<Line> > areas;


};


class Config {
public:

    std::vector<std::string> labels;
};

#endif // STATE_H
