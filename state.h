#ifndef STATE_H
#define STATE_H

#include <memory>
#include <QPoint>
#include <QLine>
#include <QString>

struct Rect {

    Rect (QLineF side1, QLineF side2, int label) \
        : side1(side1), side2(side2), label(label) {}

    Rect() : label(0) {}

    QLineF side1;
    QLineF side2;

    int label;

};

struct Point {
    QPointF p;
    float radius;
};

struct Line {
    Point start;
    Point end;

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
