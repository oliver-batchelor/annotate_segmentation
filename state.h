#ifndef STATE_H
#define STATE_H

#include <memory>
#include <QPoint>

struct Section {

    QPointF pos;
    float width;
};

struct Area {
    std::vector<Section> sections;
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
