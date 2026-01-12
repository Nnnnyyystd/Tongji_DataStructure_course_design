#pragma once

#include <QGraphicsObject>
#include <QVector>
#include <QCursor>
#include "socialgraph.h"

class EdgeItem;

class NodeItem : public QGraphicsObject
{
    Q_OBJECT
public:
    enum class Role { Current, Friend, Suggest, Other };   // 四态

    NodeItem(PersonId id, const QString& name, Role role,
             QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void   paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) override;

    void addEdge(EdgeItem* e) { if (e) edges_.push_back(e); }
    void setRole(Role r) { role_ = r; update(); }

    PersonId id()  const { return id_; }
    Role     role() const { return role_; }

signals:
    void clicked(PersonId id);
    void editRequested(PersonId id);            // 双击触发

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* ev) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* ev) override;

private:
    static constexpr qreal R = 36.0;  // 半径

    PersonId id_{0};
    QString  label_;
    Role     role_{Role::Other};
    QVector<EdgeItem*> edges_;
};
