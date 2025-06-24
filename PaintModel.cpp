#include "PaintModel.h"

void PaintModel::addPoint(POINT p)
{
    points_.push_back(p);
}

const std::vector<POINT> &PaintModel::getPoints() const
{
    return points_;
}

void PaintModel::clearPoints()
{
    points_.clear();
}