#ifndef NODE_ITEM_H
#define NODE_ITEM_H

#include <QObject>
#include <QGraphicsEllipseItem>
#include <QColor>

class QGraphicsSimpleTextItem;
class ThreadedNode;

// 既要信号又要图形项：多继承 QObject + QGraphicsEllipseItem
class NodeItem : public QObject, public QGraphicsEllipseItem
{
    Q_OBJECT
public:
    explicit NodeItem(ThreadedNode* n, QGraphicsItem* parent = nullptr);

    ThreadedNode* node() const { return m_node; }

    // 半径：供外部需要时使用（例如布局/碰撞计算）
    static constexpr qreal radius() { return R; }

    // 圆心（场景坐标）
    QPointF centerScene() const;

    // 从本节点指向目标点 dst（场景坐标）时，返回“圆周上的锚点”。
    // 用它作为连线端点，线就不会穿过节点。
    QPointF anchorTowards(const QPointF& dst) const;

    // 高亮/还原（遍历动画、选中等）
    void setHighlighted(bool on, const QColor& fill = QColor("#FFD86E"));
    void setStrokeColor(const QColor& c, qreal width = 2.0);

    //设置节点内文字（例如编号）
    void setLabel(const QString& text);
    void setLabelColor(const QColor& c);

signals:
    void clicked(ThreadedNode* n);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* ev) override;

private:
    ThreadedNode* m_node;
    QGraphicsSimpleTextItem* m_label = nullptr;

    static constexpr qreal R = 10;   // 节点圆半径（稍微比原来 10 大一点更清晰）
    QColor m_normalFill = Qt::white;
    QColor m_highlightFill = QColor("#FFD86E");
};

#endif // NODE_ITEM_H
