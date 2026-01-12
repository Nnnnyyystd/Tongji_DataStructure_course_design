#include "binarytree.h"
#include <QtMath>


BinaryTree::~BinaryTree()
{
    clear();
}

void BinaryTree::clear()
{
    destroy(m_root);
    m_root = nullptr;
}

void BinaryTree::destroy(ThreadedNode* p)
{
    if (!p) return;

    // 先把线索恢复为空，避免把线索当孩子递归
    if (p->ltag == ThreadedNode::Thread) p->left  = nullptr;
    if (p->rtag == ThreadedNode::Thread) p->right = nullptr;

    destroy(p->left);
    destroy(p->right);
    delete p;
}


ThreadedNode* BinaryTree::buildFullByHeight(int height)
{
    clear();
    if (height <= 0) return m_root;      // 空树
    int nextVal = 1;                     // 给节点编号，便于 UI 展示
    m_root = buildFullRec(/*curr*/1, height, /*parent*/nullptr, nextVal);
    return m_root;
}

ThreadedNode* BinaryTree::buildFullRec(int curr, int max, ThreadedNode* parent, int& nextVal)
{
    if (curr > max) return nullptr;

    auto* node = new ThreadedNode(nextVal++, parent);

    // 满二叉树：除最后一层外必有左右孩子
    node->setLeftChild ( buildFullRec(curr + 1, max, node, nextVal) );
    node->setRightChild( buildFullRec(curr + 1, max, node, nextVal) );

    // 线索标记默认是 Child，保持不动
    return node;
}

// 叶子计数
int BinaryTree::leafCount() const
{
    return leafCountRec(m_root);
}

int BinaryTree::leafCountRec(ThreadedNode* p) const
{
    if (!p) return 0;
    if (p->isLeaf()) return 1;
    // 注意：线索不算孩子
    int l = (p->ltag == ThreadedNode::Child) ? leafCountRec(p->left)  : 0;
    int r = (p->rtag == ThreadedNode::Child) ? leafCountRec(p->right) : 0;
    return l + r;
}

// 普通递归遍历 
QString BinaryTree::preorder() const
{
    QString out;
    preorderRec(m_root, out);
    return out.trimmed();
}

QString BinaryTree::inorder() const
{
    QString out;
    inorderRec(m_root, out);
    return out.trimmed();
}

QString BinaryTree::postorder() const
{
    QString out;
    postorderRec(m_root, out);
    return out.trimmed();
}

void BinaryTree::preorderRec(ThreadedNode* p, QString& out) const
{
    if (!p) return;
    out += QString::number(p->value) + ' ';
    if (p->ltag == ThreadedNode::Child) preorderRec(p->left,  out);
    if (p->rtag == ThreadedNode::Child) preorderRec(p->right, out);
}

void BinaryTree::inorderRec(ThreadedNode* p, QString& out) const
{
    if (!p) return;
    if (p->ltag == ThreadedNode::Child) inorderRec(p->left,  out);
    out += QString::number(p->value) + ' ';
    if (p->rtag == ThreadedNode::Child) inorderRec(p->right, out);
}

void BinaryTree::postorderRec(ThreadedNode* p, QString& out) const
{
    if (!p) return;
    if (p->ltag == ThreadedNode::Child) postorderRec(p->left,  out);
    if (p->rtag == ThreadedNode::Child) postorderRec(p->right, out);
    out += QString::number(p->value) + ' ';
}

// 线索化
// 线索化的统一原则：在“访问到当前结点 p”时：
//  1) 如果 p->left 为空，则让 left 指向“本遍历序的前驱 pre”，并置 ltag=Thread；
//  2) 如果 pre 不为空且 pre->right 为空，则让 pre->right 指向“本遍历序的后继 p”，并置 pre->rtag=Thread；
//  3) 最后 pre = p；
// 遍历序（先/中/后）不同，得到的前驱/后继关系不同，这正是“线索化”的本质。

void BinaryTree::clearThreadsRec(ThreadedNode* p)
{
    if (!p) return;
    p->clearThreads(); // 只清本节点的线索标记与指针
    if (p->ltag == ThreadedNode::Child) clearThreadsRec(p->left);
    if (p->rtag == ThreadedNode::Child) clearThreadsRec(p->right);
}

void BinaryTree::preorderThreadRec(ThreadedNode* p, ThreadedNode*& pre)
{
    if (!p) return;

    // —— 访问 p（先序）——
    if (!p->left)  p->setLeftThread(pre);
    if (pre && !pre->right) pre->setRightThread(p);
    pre = p;

    if (p->ltag == ThreadedNode::Child) preorderThreadRec(p->left, pre);
    if (p->rtag == ThreadedNode::Child) preorderThreadRec(p->right, pre);
}

void BinaryTree::inorderThreadRec(ThreadedNode* p, ThreadedNode*& pre)
{
    if (!p) return;

    if (p->ltag == ThreadedNode::Child) inorderThreadRec(p->left, pre);

    // —— 访问 p（中序）——
    if (!p->left)  p->setLeftThread(pre);
    if (pre && !pre->right) pre->setRightThread(p);
    pre = p;

    if (p->rtag == ThreadedNode::Child) inorderThreadRec(p->right, pre);
}

void BinaryTree::postorderThreadRec(ThreadedNode* p, ThreadedNode*& pre)
{
    if (!p) return;

    if (p->ltag == ThreadedNode::Child) postorderThreadRec(p->left, pre);
    if (p->rtag == ThreadedNode::Child) postorderThreadRec(p->right, pre);

    // —— 访问 p（后序）——
    if (!p->left)  p->setLeftThread(pre);
    if (pre && !pre->right) pre->setRightThread(p);
    pre = p;
}

void BinaryTree::makePreorderThread()
{
    if (!m_root) return;
    clearThreadsRec(m_root);               // 清理旧线索
    ThreadedNode* pre = nullptr;
    preorderThreadRec(m_root, pre);
}

void BinaryTree::makeInorderThread()
{
    if (!m_root) return;
    clearThreadsRec(m_root);
    ThreadedNode* pre = nullptr;
    inorderThreadRec(m_root, pre);
}

void BinaryTree::makePostorderThread()
{
    if (!m_root) return;
    clearThreadsRec(m_root);
    ThreadedNode* pre = nullptr;
    postorderThreadRec(m_root, pre);
}

// 线索遍历
// 中序线索遍历：从整棵树的“最左”开始，依次按“线索后继/右子树最左”推进
QString BinaryTree::inorderThreadedWalk() const
{
    if (!m_root) return {};
    QString out;

    const ThreadedNode* p = m_root->firstInorder();
    while (p) {
        out += QString::number(p->value) + ' ';

        if (p->rtag == ThreadedNode::Thread) {
            p = p->right;                 // 线索后继
        } else {
            if (!p->right) break;
            const ThreadedNode* q = p->right;
            while (q && q->ltag == ThreadedNode::Child && q->left)
                q = q->left;              // 右子树的最左
            p = q;
        }
    }
    return out.trimmed();
}

// 先序线索遍历
//    访问 p -> 如果有“左孩子”，下一步到左孩子；否则“右指针”就是下一步（要么是右孩子，要么是线索后继）。
QString BinaryTree::preorderThreadedWalk() const
{
    if (!m_root) return {};
    QString out;

    const ThreadedNode* p = m_root;
    while (p) {
        out += QString::number(p->value) + ' ';
        if (p->ltag == ThreadedNode::Child && p->left) {
            p = p->left;
        } else {
            p = p->right; // 可能是右孩子，也可能是线索后继（我们在线索化时已把“无右孩子”的情况接成后继）
        }
    }
    return out.trimmed();
}

// 删除叶子
bool BinaryTree::removeLeaf(ThreadedNode* n)
{
    if (!n || !m_root) return false;
    if (!n->isLeaf())  return false;

    ThreadedNode* p = n->parent;
    if (!p) {  // 根且是唯一节点
        delete n; m_root = nullptr; return true;
    }
    if (p->left == n)  p->left = nullptr;
    if (p->right == n) p->right = nullptr;
    delete n;
    return true;
}
