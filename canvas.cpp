#include "canvas.h"


Canvas::Canvas(std::shared_ptr<State> state) : state(state) {
    setMouseTracking(true);
}



void Canvas::paintEvent(QPaintEvent * /* event */) {
    QPainter painter(this);


    for(auto a : state->areas) {



    }


}
