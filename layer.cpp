#include "layer.h"
#include <set>



QPixmap const& Layer::getPixmap() {

    if(dirty) {

        QImage indexMask(image.data, image.cols, image.rows, QImage::Format_Indexed8);
        indexMask.setColorTable(palette);

        pixmap = QPixmap::fromImage(indexMask);
    }

    return pixmap;

}


float length(cv::Point2f const &p) {
    return std::sqrt(p.dot(p));
}


inline cv::Point2f perpLine(cv::Point2f const &start, cv::Point2f const &end) {
    cv::Point2f line = end - start;
    cv::Point2f dir = line / length(line);

    return cv::Point2f (dir.y, -dir.x);
}


inline std::vector<cv::Point2f> makeRect(Point const &p1, Point const p2) {
    cv::Point2f perp = perpLine(p1.p, p2.p);
    return std::vector<cv::Point2f> {p1.p + perp * p1.r, p2.p + perp * p2.r, p2.p - perp * p2.r, p1.p - perp * p1.r};
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


void Layer::drawLine(Point const &start, Point const& end, int label) {
    std::vector<cv::Point2f> rect = makeRect(start, end);
    std::vector<cv::Point> points(rect.begin(), rect.end());

    cv::Scalar c(label, label, label);
    cv::fillConvexPoly(image, points, c);

    drawPoint(start, label);
    drawPoint(end, label);
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



QVector<QRgb> makeColorTable() {

    QVector<QRgb> colors = {
        0xFFFF00, 0x1CE6FF, 0xFF34FF, 0xFF4A46, 0x008941, 0x006FA6, 0xA30059,
        0xFFDBE5, 0x7A4900, 0x0000A6, 0x63FFAC, 0xB79762, 0x004D43, 0x8FB0FF, 0x997D87,
        0x5A0007, 0x809693, 0xFEFFE6, 0x1B4400, 0x4FC601, 0x3B5DFF, 0x4A3B53, 0xFF2F80,
        0x61615A, 0xBA0900, 0x6B7900, 0x00C2A0, 0xFFAA92, 0xFF90C9, 0xB903AA, 0xD16100,
        0xDDEFFF, 0x000035, 0x7B4F4B, 0xA1C299, 0x300018, 0x0AA6D8, 0x013349, 0x00846F,
        0x372101, 0xFFB500, 0xC2FFED, 0xA079BF, 0xCC0744, 0xC0B9B2, 0xC2FF99, 0x001E09,
        0x00489C, 0x6F0062, 0x0CBD66, 0xEEC3FF, 0x456D75, 0xB77B68, 0x7A87A1, 0x788D66,
        0x885578, 0xFAD09F, 0xFF8A9A, 0xD157A0, 0xBEC459, 0x456648, 0x0086ED, 0x886F4C,
        0x34362D, 0xB4A8BD, 0x00A6AA, 0x452C2C, 0x636375, 0xA3C8C9, 0xFF913F, 0x938A81,
        0x575329, 0x00FECF, 0xB05B6F, 0x8CD0FF, 0x3B9700, 0x04F757, 0xC8A1A1, 0x1E6E00,
        0x7900D7, 0xA77500, 0x6367A9, 0xA05837, 0x6B002C, 0x772600, 0xD790FF, 0x9B9700,
        0x549E79, 0xFFF69F, 0x201625, 0x72418F, 0xBC23FF, 0x99ADC0, 0x3A2465, 0x922329,
        0x5B4534, 0xFDE8DC, 0x404E55, 0x0089A3, 0xCB7E98, 0xA4E804, 0x324E72, 0x6A3A4C,
        0x83AB58, 0x001C1E, 0xD1F7CE, 0x004B28, 0xC8D0F6, 0xA3A489, 0x806C66, 0x222800,
        0xBF5650, 0xE83000, 0x66796D, 0xDA007C, 0xFF1A59, 0x8ADBB4, 0x1E0200, 0x5B4E51,
        0xC895C5, 0x320033, 0xFF6832, 0x66E1D3, 0xCFCDAC, 0xD0AC94, 0x7ED379, 0x012C58,
        0x7A7BFF, 0xD68E01, 0x353339, 0x78AFA1, 0xFEB2C6, 0x75797C, 0x837393, 0x943A4D,
        0xB5F4FF, 0xD2DCD5, 0x9556BD, 0x6A714A, 0x001325, 0x02525F, 0x0AA3F7, 0xE98176,
        0xDBD5DD, 0x5EBCD1, 0x3D4F44, 0x7E6405, 0x02684E, 0x962B75, 0x8D8546, 0x9695C5,
        0xE773CE, 0xD86A78, 0x3E89BE, 0xCA834E, 0x518A87, 0x5B113C, 0x55813B, 0xE704C4,
        0x00005F, 0xA97399, 0x4B8160, 0x59738A, 0xFF5DA7, 0xF7C9BF, 0x643127, 0x513A01,
        0x6B94AA, 0x51A058, 0xA45B02, 0x1D1702, 0xE20027, 0xE7AB63, 0x4C6001, 0x9C6966,
        0x64547B, 0x97979E, 0x006A66, 0x391406, 0xF4D749, 0x0045D2, 0x006C31, 0xDDB6D0,
        0x7C6571, 0x9FB2A4, 0x00D891, 0x15A08A, 0xBC65E9, 0xFFFFFE, 0xC6DC99, 0x203B3C,
        0x671190, 0x6B3A64, 0xF5E1FF, 0xFFA0F2, 0xCCAA35, 0x374527, 0x8BB400, 0x797868,
        0xC6005A, 0x3B000A, 0xC86240, 0x29607C, 0x402334, 0x7D5A44, 0xCCB87C, 0xB88183,
        0xAA5199, 0xB5D6C3, 0xA38469, 0x9F94F0, 0xA74571, 0xB894A6, 0x71BB8C, 0x00B433,
        0x789EC9, 0x6D80BA, 0x953F00, 0x5EFF03, 0xE4FFFC, 0x1BE177, 0xBCB1E5, 0x76912F,
        0x003109, 0x0060CD, 0xD20096, 0x895563, 0x29201D, 0x5B3213, 0xA76F42, 0x89412E,
        0x1A3A2A, 0x494B5A, 0xA88C85, 0xF4ABAA, 0xA3F3AB, 0x00C6C8, 0xEA8B66, 0x958A9F,
        0xBDC9D2, 0x9FA064, 0xBE4700, 0x658188, 0x83A485, 0x453C23, 0x47675D, 0x3A3F00,
        0x061203, 0xDFFB71, 0x868E7E, 0x98D058, 0x6C8F7D, 0xD7BFC2, 0x3C3E6E, 0xD83D66,
        0x2F5D9B, 0x6C5E46, 0xD25B88, 0x5B656C, 0x00B57F, 0x545C46, 0x866097, 0x365D25,
        0x252F99, 0x00CCFF, 0x674E60, 0xFC009C, 0x92896B
    };

//    for(int i = 0; i < colors.size(); ++i) {
//        QColor col(colors[i]);
//        col.setAlpha(255);

//        colors[i] = col.rgb();
//    }


    return colors;
}
