#include "edgeitem.h"
#include "nodeitem.h"
#include <QPen>
#include <QLineF>

EdgeItem::EdgeItem(NodeItem* a, NodeItem* b, QGraphicsItem* parent)
    : QGraphicsLineItem(parent), a_(a), b_(b)
{
    setZValue(0);
    setPen(QPen(QColor(138, 84, 28), 3)); // 棕色线
    a_->addEdge(this);
    b_->addEdge(this);
    adjust();
}

void EdgeItem::adjust()
{
    QPointF c1 = a_->sceneBoundingRect().center();
    QPointF c2 = b_->sceneBoundingRect().center();

    QLineF l(c1, c2);
    if (l.length() > 1.0) {
        // 稍微缩短到圆边缘，看起来更自然
        const qreal shrink = 18.0;
        l.setLength(l.length() - shrink);
        l.setP1(l.pointAt(shrink / l.length()));
    }
    setLine(l);
}
