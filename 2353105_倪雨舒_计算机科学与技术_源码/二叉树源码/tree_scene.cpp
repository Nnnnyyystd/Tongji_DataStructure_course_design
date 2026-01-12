#include "tree_scene.h"
#include "threadednode.h"
#include "node_item.h"

#include <QGraphicsPathItem>
#include <QPen>
#include <QBrush>
#include <QtMath>

TreeScene::TreeScene(QObject* parent)
    : QGraphicsScene(parent)
{
    setBackgroundBrush(QColor("#f7f7f7"));
}

void TreeScene::renderTree(ThreadedNode* root)
{
    // 先处理覆盖物，避免
    clearOverlays();

    clear();                // 删除所有图元（节点、线等）
    m_items.clear();

    if (!root) return;

    layoutCompact(root);
    drawDfs(root);

    setSceneRect(itemsBoundingRect().adjusted(-40,-40,40,40));
}



void TreeScene::layoutCompact(ThreadedNode* root)
{
    double leafCursor = 0.0;
    layoutAssignX(root, /*depth*/0, leafCursor);  // 设 x
    applyYByDepth(root, /*depth*/0);              // 设 y
}

// 返回该节点的 x；顺手把 p->posHint.x() 设好
double TreeScene::layoutAssignX(ThreadedNode* p, int depth, double& leafCursor)
{
    if (!p) return 0.0;

    const bool hasL = (p->ltag == ThreadedNode::Child && p->left);
    const bool hasR = (p->rtag == ThreadedNode::Child && p->right);

    double x = 0.0;
    if (!hasL && !hasR) {
        const double dx = m_dxBase * qPow(m_shrink, depth);
        x = leafCursor;
        leafCursor += dx;
    } else {
        double xl = 0.0, xr = 0.0;
        if (hasL) xl = layoutAssignX(p->left,  depth + 1, leafCursor);
        if (hasR) xr = layoutAssignX(p->right, depth + 1, leafCursor);

        if (hasL && hasR)      x = (xl + xr) * 0.5;
        else if (hasL)         x = xl;
        else                   x = xr;
    }

    p->posHint.setX(x);
    return x;
}

void TreeScene::applyYByDepth(ThreadedNode* p, int depth)
{
    if (!p) return;
    p->posHint.setY(depth * m_dy);
    if (p->ltag == ThreadedNode::Child) applyYByDepth(p->left,  depth + 1);
    if (p->rtag == ThreadedNode::Child) applyYByDepth(p->right, depth + 1);
}

/* ================== 图元与绘制 ================== */

NodeItem* TreeScene::ensureNodeItem(ThreadedNode* p)
{
    if (m_items.contains(p)) return m_items[p];

    auto* item = new NodeItem(p);
    addItem(item);
    item->setPos(p->posHint);
    item->setZValue(1);                 // 节点在上层

    // 在节点内放编号，省去外部文字图元
    item->setLabel(QString::number(p->value));

    connect(item, &NodeItem::clicked, this, &TreeScene::nodeClicked);
    m_items.insert(p, item);
    return item;
}

void TreeScene::drawDfs(ThreadedNode* p)
{
    if (!p) return;

    auto* me = ensureNodeItem(p);

    auto drawEdge = [&](ThreadedNode* child) {
        auto* ci = ensureNodeItem(child);

        // 端点从“圆周锚点”取得，避免穿过节点
        const QPointF src = me->anchorTowards(ci->centerScene());
        const QPointF dst = ci->anchorTowards(me->centerScene());

        auto* line = addLine(QLineF(src, dst), QPen(Qt::black, 1.6));
        line->setZValue(0);                 // 线在下层
        m_overlays << line;                 // 作为“基础边”是否放 overlays 可按需，这里放入便于统一清理
    };

    if (p->ltag == ThreadedNode::Child && p->left)  { drawEdge(p->left);  drawDfs(p->left);  }
    if (p->rtag == ThreadedNode::Child && p->right) { drawEdge(p->right); drawDfs(p->right); }
}

/* ================== 覆盖物 & 高亮 ================== */

void TreeScene::clearOverlays()
{
    for (auto *it : m_overlays) {
        if (!it) continue;
        if (it->scene() == this) removeItem(it);
        delete it;
    }
    m_overlays.clear();

    // 还原节点高亮
    for (auto* it : m_items) it->setHighlighted(false);
}

void TreeScene::clearHighlightsOnly()
{
    for (auto* it : m_items) it->setHighlighted(false);
}

void TreeScene::highlightNode(ThreadedNode* n, bool on, const QColor& fill)
{
    if (!n) return;
    if (auto* it = itemOf(n)) it->setHighlighted(on, fill);
}

/* ================== 箭头（线索/遍历辅助） ================== */

void TreeScene::addArrow(ThreadedNode* from, ThreadedNode* to,
                         const QColor& color, bool dashed)
{
    if (!from || !to) return;
    auto* a = itemOf(from);
    auto* b = itemOf(to);
    if (!a || !b) return;

    const QPointF p1 = a->anchorTowards(b->centerScene());
    const QPointF p2 = b->anchorTowards(a->centerScene());

    // 主干
    QPen pen(color, 2, dashed ? Qt::DashLine : Qt::SolidLine);
    pen.setCosmetic(true);
    auto* line = addLine(QLineF(p1, p2), pen);
    line->setZValue(0);
    m_overlays << line;

    // 箭头
    const qreal head    = 10.0;                 // 箭头长度
    const qreal degrees = 25.0;                 // 两翼角
    QLineF base(p2, p1);
    base.setLength(head);

    QLineF lwing = base; lwing.setAngle(base.angle() - degrees);
    QLineF rwing = base; rwing.setAngle(base.angle() + degrees);

    auto* l1 = addLine(QLineF(p2, p2 + (lwing.p2() - lwing.p1())), QPen(color, 2));
    auto* l2 = addLine(QLineF(p2, p2 + (rwing.p2() - rwing.p1())), QPen(color, 2));
    l1->setZValue(0); l2->setZValue(0);
    m_overlays << l1 << l2;
}
