#ifndef TREE_SCENE_H
#define TREE_SCENE_H

#include <QGraphicsScene>
#include <QMap>
#include <QList>
#include <QColor>

class ThreadedNode;
class NodeItem;

class TreeScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit TreeScene(QObject* parent = nullptr);

    // 渲染整棵树（会清空 scene 与 item 映射；覆盖物也会清理）
    void renderTree(ThreadedNode* root);

    // 展示页：临时覆盖物/高亮控制
    void clearOverlays();                                           // 清临时覆盖物（箭头等）
    void clearHighlightsOnly();                                     // 只还原高亮
    void highlightNode(ThreadedNode* n, bool on,
                       const QColor& fill = QColor("#FFE08A"));     // 高亮/恢复
    void addArrow(ThreadedNode* from, ThreadedNode* to,
                  const QColor& color = QColor("#0080FF"),
                  bool dashed = true);                              // 画箭头

signals:
    void nodeClicked(ThreadedNode* n);

private:
    // —— 布局（两遍）——
    void layoutCompact(ThreadedNode* root);
    double layoutAssignX(ThreadedNode* p, int depth, double& leafCursor);
    void applyYByDepth(ThreadedNode* p, int depth);

    // —— 绘制 ——
    void drawDfs(ThreadedNode* p);
    NodeItem* ensureNodeItem(ThreadedNode* p);
    NodeItem* itemOf(ThreadedNode* p) const { return m_items.value(p, nullptr); }

private:
    // 布局参数
    double m_dxBase   = 80.0;   // 叶子在第0层（根的层为0）基础水平间距
    double m_shrink   = 0.85;   // 每深入一层水平间距乘该因子（越小越“瘦”）
    double m_dy       = 110.0;  // 垂直层距

    QMap<ThreadedNode*, NodeItem*> m_items;
    QList<QGraphicsItem*> m_overlays;      // 箭头、高亮辅助形状
};

#endif // TREE_SCENE_H
