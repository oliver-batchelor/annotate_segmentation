#ifndef STATE_H
#define STATE_H

#include <memory>
#include <QPoint>
#include <QLine>
#include <QString>



struct Area {
    std::vector<QLineF> sections;
    int label;
};


class State
{
public:
    State() : currentLabel(0) {}


    QString filename;
    std::vector<Area> areas;

    int currentLabel;


};


class Config {
public:

    std::vector<std::string> labels;
};

#endif // STATE_H
