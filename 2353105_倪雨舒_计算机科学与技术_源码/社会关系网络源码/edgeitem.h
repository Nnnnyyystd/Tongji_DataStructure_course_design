#pragma once
#include <QGraphicsLineItem>

class NodeItem;

class EdgeItem : public QGraphicsLineItem
{
public:
    EdgeItem(NodeItem* a, NodeItem* b, QGraphicsItem* parent = nullptr);
    void adjust();
private:
    NodeItem* a_{nullptr};
    NodeItem* b_{nullptr};
};
