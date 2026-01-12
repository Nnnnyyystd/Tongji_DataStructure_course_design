#include "node_item.h"
#include "threadednode.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSimpleTextItem>
#include <QBrush>
#include <QPen>
#include <QtMath>

NodeItem::NodeItem(ThreadedNode* n, QGraphicsItem* parent)
    : QGraphicsEllipseItem(parent), m_node(n)
{
    setRect(-R, -R, 2*R, 2*R);
    setBrush(m_normalFill);
    setPen(QPen(Qt::black, 2));
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setZValue(1);                      // 节点在连线之上


    m_label = new QGraphicsSimpleTextItem(this);
    m_label->setBrush(QBrush(Qt::black));
    m_label->setText(QString::number(n ? n->value : 0));
    // 文本居中：简单处理——设置锚点到圆心附近
    const QRectF br = m_label->boundingRect();
    m_label->setPos(-br.width()/2.0, -br.height()/2.0);
}

QPointF NodeItem::centerScene() const
{
    // 项本地坐标(0,0)就是圆心；映射到场景
    return mapToScene(QPointF(0,0));
}

QPointF NodeItem::anchorTowards(const QPointF& dst) const
{
    const QPointF c = centerScene();
    QPointF v = dst - c;
    const qreal len = std::hypot(v.x(), v.y());
    if (len < 1e-6) return c;       // 与目标重合，直接返回中心
    const QPointF dir = v / len;
    return c + dir * R;             // 圆周上的锚点
}

void NodeItem::setHighlighted(bool on, const QColor& fill)
{
    if (on) {
        m_highlightFill = fill;
        setBrush(m_highlightFill);
        setPen(QPen(QColor("#E67E22"), 2.2)); // 高亮时描边微调
    } else {
        setBrush(m_normalFill);
        setPen(QPen(Qt::black, 2));
    }
    update();
}

void NodeItem::setStrokeColor(const QColor& c, qreal width)
{
    setPen(QPen(c, width));
    update();
}

void NodeItem::setLabel(const QString& text)
{
    if (!m_label) return;
    m_label->setText(text);
    const QRectF br = m_label->boundingRect();
    m_label->setPos(-br.width()/2.0, -br.height()/2.0);
}

void NodeItem::setLabelColor(const QColor& c)
{
    if (!m_label) return;
    m_label->setBrush(QBrush(c));
}

void NodeItem::mousePressEvent(QGraphicsSceneMouseEvent* ev)
{
    QGraphicsEllipseItem::mousePressEvent(ev);
    emit clicked(m_node);
}
