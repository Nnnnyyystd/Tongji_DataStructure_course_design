#ifndef THREADEDNODE_H
#define THREADEDNODE_H

#include <QtGlobal>
#include <QPointF>

class QGraphicsItem;   // 只做前向声明，避免引入图形模块的重编译

/**
 * 线索二叉树的“节点”——仅封装节点本身的状态与常用操作。
 * 不负责节点的生命周期管理（释放由树统一处理）。
 */
class ThreadedNode
{
public:
    // 指针语义：Child 表示真实孩子；Thread 表示线索指向前驱/后继
    enum Tag : quint8 { Child = 0, Thread = 1 };

    int value = 0;

    ThreadedNode* left   = nullptr;
    ThreadedNode* right  = nullptr;
    ThreadedNode* parent = nullptr;

    Tag ltag = Child;
    Tag rtag = Child;

    // 
    QGraphicsItem* gfx = nullptr;   // 绑定到场景中的图元
    QPointF         posHint;        // 预布局坐标（可选）
    bool            selected = false;

public:
    explicit ThreadedNode(int v = 0, ThreadedNode* p = nullptr);

    // 结构类工具
    bool hasLeftChild()  const;   // 左孩子存在且为 Child
    bool hasRightChild() const;   // 右孩子存在且为 Child
    bool isLeaf()        const;   // 无真实左右孩子（线索不算孩子）
    bool deletableLeaf() const;   // 是否安全作为“叶子”删除

    // 线索存在性（用于可视化与遍历判定）
    bool hasLeftThread()  const { return ltag == Thread && left  != nullptr; }
    bool hasRightThread() const { return rtag == Thread && right != nullptr; }

    // 修改关系（统一维护 parent 与标记）
    void setLeftChild (ThreadedNode* child);
    void setRightChild(ThreadedNode* child);

    // 线索设置/清理（不会动 parent）
    void setLeftThread (ThreadedNode* predecessor);
    void setRightThread(ThreadedNode* successor);
    void clearThreads();                    // 如果当前为线索则清空并还原为 Child

    
    static void ClearAllThreads(ThreadedNode* root);           // 递归清整棵树线索
    static void ResetParent(ThreadedNode* root,
                            ThreadedNode* parent = nullptr);   // 递归刷新 parent

    // 可视化辅助
    void bindGraphics(QGraphicsItem* item) { gfx = item; }
    void setSelected(bool on) { selected = on; }

    // —— 中序导航：结合线索与孩子，做局部邻接查找 ——
    ThreadedNode* firstInorder();        // 以当前结点为根找到最左端
    ThreadedNode* inorderSuccessor();    // 中序后继（考虑线索）
    ThreadedNode* inorderPredecessor();  // 中序前驱（考虑线索）
};

#endif // THREADEDNODE_H
