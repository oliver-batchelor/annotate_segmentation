#include "layer.h"



QPixmap const& Layer::getPixmap() {

    if(dirty) {

        QImage indexMask(edit.data, edit.cols, edit.rows, QImage::Format_Indexed8);
        indexMask.setColorTable(palette);

        pixmap = QPixmap::fromImage(indexMask);
    }

    return pixmap;

}



void Layer::drawPoint(Point const &p, int label) {
    cv::Scalar c(label, label, label);
    cv::circle(image, cv::Point(p.p.x, p.p.y), p.r, c, -1);

    dirty = true;
}

void Layer::drawPoly(std::vector<cv::Point2f> const &points, int label) {
    cv::Scalar c(label, label, label);

    std::vector<cv::Point> ps;
    for(auto const& p : points) {
        ps.push_back(p);
    }


    std::vector<std::vector<cv::Point>> pts = {ps};
    cv::fillPoly(image, pts, c);

}

void Layer::drawSP(cv::Mat1i const& spLabels, Point const &p, int label) {

    cv::Point c = p.p;
    int r = p.r;

    std::set<int> labels;
    for(int i = -p.r; i <= r; ++i) {
        for(int j = -p.r; j <= r; ++j) {
            if(i * i + j * j < r * r) {
                int x = c.x + j;
                int y = c.y + i;

                if(y < spLabels.rows && x < spLabels.cols && x >= 0 && y >= 0) {
                    int spLabel = spLabels(y, x);
                    labels.insert(spLabel);
                }
            }
        }
    }



    for (int l : labels) {
        cv::Mat1b mask = spLabels == l;
        image.setTo(label, mask);
    }

    dirty = true;
}



void Layer::floodFill(Point const &p, int label) {
    cv::Scalar c(label, label, label);
    cv::floodFill(image, cv::Point(p.p.x, p.p.y), c);

    dirty = true;
}



void Layer::drawRect(cv::Rect2f const &s, int label) {
    image(s) = label;

    dirty = true;
}

