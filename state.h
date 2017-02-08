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
    typedef std::shared_ptr<State> Ptr;

    State();


    std::string filename;
    std::vector<Area> areas;

};

#endif // STATE_H
