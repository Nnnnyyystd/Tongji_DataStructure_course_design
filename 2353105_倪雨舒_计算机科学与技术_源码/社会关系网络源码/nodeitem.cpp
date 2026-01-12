#include "nodeitem.h"
#include "edgeitem.h"

#include <QPainter>
#include <QGraphicsSceneMouseEvent>

NodeItem::NodeItem(PersonId id, const QString& name, Role role, QGraphicsItem* parent)
    : QGraphicsObject(parent), id_(id), label_(name), role_(role)
{
    setFlags(ItemIsMovable | ItemSendsGeometryChanges | ItemIsSelectable);
    setZValue(1);
    setCursor(Qt::PointingHandCursor);
}

QRectF NodeItem::boundingRect() const
{
    return QRectF(-R, -R, 2*R, 2*R);
}

void NodeItem::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*)
{
    // 统一配色
    QColor c;
    switch (role_) {
    case Role::Current: c = QColor(255, 99, 99);   break;  // 红
    case Role::Friend:  c = QColor(255, 215, 0);   break;  // 黄 gold
    case Role::Suggest: c = QColor(144, 238, 144); break;  // 绿 lightgreen
    case Role::Other:   c = QColor(135, 206, 250); break;  // 蓝 lightskyblue
    }

    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(Qt::NoPen);
    p->setBrush(c);
    p->drawEllipse(boundingRect());

    // 中间白字标签
    p->setPen(Qt::white);
    QFont f = p->font();
    f.setBold(true);
    p->setFont(f);
    const QFontMetrics fm(f);
    const int w = fm.horizontalAdvance(label_);
    const int h = fm.ascent();
    p->drawText(-w/2, h/2, label_);
}

QVariant NodeItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionHasChanged) {
        for (auto* e : edges_) {
            if (e) e->adjust();
        }
    }
    return QGraphicsObject::itemChange(change, value);
}

void NodeItem::mousePressEvent(QGraphicsSceneMouseEvent* ev)
{
    if (ev->button() == Qt::LeftButton)
        emit clicked(id_);
    QGraphicsObject::mousePressEvent(ev);
}

void NodeItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* ev)
{
    emit editRequested(id_);
    ev->accept();

}
