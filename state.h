#ifndef STATE_H
#define STATE_H

#include <memory>

struct Area {

    QPoint start, end;
    float width;

};


class State
{
public:
    State();


    std::string filename;
    std::vector<Area> areas;

};

#endif // STATE_H
