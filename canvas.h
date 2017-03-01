#ifndef CANVAS_H
#define CANVAS_H


#include <QWidget>
#include "state.h"

#include <boost/optional.hpp>
#include <boost/variant.hpp>

struct Draw {
    Point p;
};

struct End {
    Point p;
};

struct Cancel {
};

struct Deletion {
    int index;
    Area a;
};

struct Delete {
    std::vector<Deletion> deletes;
};

typedef boost::variant <Draw, End, Cancel, Delete> Command;

inline Command drawCmd(Point const &p) {
    Draw d;
    d.p = p;
    return Command(d);
}

inline Command endCmd(Point const &p) {
    End e;
    e.p = p;
    return Command(e);
}

inline Command cancelCmd() {
    Cancel c;
    return Command(c);
}

inline Command deleteCmd(std::vector<Deletion> const& dels) {
    Delete c;
    c.deletes = dels;

    return Command(c);
}



class Canvas : public QWidget
{
    Q_OBJECT

public:
    Canvas(std::shared_ptr<State> _state);

    void setImage(QPixmap const& p);
    QImage save();

    bool isModified() { return undos.size() || redos.size(); }

public slots:
    void zoom(float zoom);

    void setLabel(int label);
    void setBrushWidth(int width);


    void undo();
    void redo();
    void cancel();

    void deleteSelection();


protected:

    void run(Command const &c);
    void setPoint(QPointF const &p);

    //    bool event(QEvent *event);
    //void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);


    void applyCmd(Command const& c);
    void undoCmd(Command const& c);

    bool isSelected(Area const& a);



private:
    void mouseMove(QMouseEvent *event);


    std::shared_ptr<State> state;

    Area currentArea;
    Point currentPoint;

    int currentLabel;


    boost::optional<QRectF> selection;
    boost::optional<QPointF> selecting;

    QPixmap image;
    QPixmap scaled;

    float scale;

    std::vector<Command> undos;
    std::vector<Command> redos;
};

#endif // CANVAS_H
