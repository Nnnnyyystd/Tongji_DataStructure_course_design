#ifndef BINARYTREE_H
#define BINARYTREE_H

#include <QtGlobal>
#include <QString>
#include <vector>
#include "threadednode.h"

/**
 * 纯“数据结构层”的二叉树：
 * - buildFullByHeight(h): 依据层高建立满二叉树（h>=1）
 * - clear(): 释放整棵树
 * - 遍历：先/中/后序（递归），返回 QString（便于 UI 显示）
 * - leafCount(): 叶子结点数（不把线索当孩子）
 * - 线索化：先/中/后序；并提供中序与先序的线索遍历
 */
class BinaryTree
{
public:
    BinaryTree() = default;
    ~BinaryTree();

    ThreadedNode* root() const { return m_root; }

    // 依据层高建立满二叉树；会自动清空旧树
    ThreadedNode* buildFullByHeight(int height);

    // 清空整棵树
    void clear();

    // 统计叶子数（真实孩子，线索不算孩子）
    int leafCount() const;

    // —— 普通递归遍历（不依赖线索） ——
    QString preorder()  const;
    QString inorder()   const;
    QString postorder() const;

    // —— 线索化：会把“空指针”按给定遍历序补为 前驱/后继 线索 ——
    void makePreorderThread();   // 先序线索化
    void makeInorderThread();    // 中序线索化
    void makePostorderThread();  // 后序线索化

    // —— 线索遍历（非递归） ——
    QString inorderThreadedWalk()  const;  // 中序线索遍历
    QString preorderThreadedWalk() const;  // 先序线索遍历（简单可靠）

    // 删除叶子（真实孩子意义上的叶子）
    bool removeLeaf(ThreadedNode* n);

private:
    ThreadedNode* m_root = nullptr;

    // 释放整棵树
    void destroy(ThreadedNode* p);

    // 递归建树：当前层 curr，目标层 max（根为 1）
    ThreadedNode* buildFullRec(int curr, int max, ThreadedNode* parent, int& nextVal);

    // 遍历工具（普通）
    void preorderRec (ThreadedNode* p, QString& out)  const;
    void inorderRec  (ThreadedNode* p, QString& out)  const;
    void postorderRec(ThreadedNode* p, QString& out)  const;

    int  leafCountRec(ThreadedNode* p) const;

    // 线索化工具（所有版本都按“访问当前结点时”去补线索）
    void clearThreadsRec(ThreadedNode* p);                 // 线索化前清线索
    void preorderThreadRec (ThreadedNode* p, ThreadedNode*& pre);
    void inorderThreadRec  (ThreadedNode* p, ThreadedNode*& pre);
    void postorderThreadRec(ThreadedNode* p, ThreadedNode*& pre);
};

#endif // BINARYTREE_H
