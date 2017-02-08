#ifndef STATE_H
#define STATE_H

#include <memory>
#include <QPoint>

struct Area {

    QPointF start, end;
    float width;

    int label;
};


class State
{
public:
    State();


    std::string filename;
    std::vector<Area> areas;


};


class Config {
public:

    std::vector<std::string> labels;
};

#endif // STATE_H
