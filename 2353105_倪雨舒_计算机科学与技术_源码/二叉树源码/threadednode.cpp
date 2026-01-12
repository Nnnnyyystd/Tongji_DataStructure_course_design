#include "threadednode.h"

ThreadedNode::ThreadedNode(int v, ThreadedNode* p)
    : value(v), parent(p)
{
}

bool ThreadedNode::hasLeftChild() const
{
    return left != nullptr && ltag == Child;
}

bool ThreadedNode::hasRightChild() const
{
    return right != nullptr && rtag == Child;
}

bool ThreadedNode::isLeaf() const
{
    return !hasLeftChild() && !hasRightChild();
}

bool ThreadedNode::deletableLeaf() const
{
    // 线索不计入孩子；删除时要求：自己是叶子
    return isLeaf();
}

void ThreadedNode::setLeftChild(ThreadedNode* child)
{
    left = child;
    ltag = Child;                 // 无论 child 是否为空，保持“孩子语义”
    if (child) child->parent = this;
}

void ThreadedNode::setRightChild(ThreadedNode* child)
{
    right = child;
    rtag = Child;
    if (child) child->parent = this;
}

void ThreadedNode::setLeftThread(ThreadedNode* predecessor)
{
    left = predecessor;
    ltag = Thread;
}

void ThreadedNode::setRightThread(ThreadedNode* successor)
{
    right = successor;
    rtag = Thread;
}

void ThreadedNode::clearThreads()
{
    if (ltag == Thread) { left  = nullptr; ltag = Child; }
    if (rtag == Thread) { right = nullptr; rtag = Child; }
}

/* 静态：整棵树清线索
 * 注意：只有在该侧为“Thread”时才清指针；如果是 Child，就递归到相应子树。
 */
void ThreadedNode::ClearAllThreads(ThreadedNode* root)
{
    if (!root) return;

    // 左侧：若是线索则清空；若是孩子则递归
    if (root->ltag == Thread) {
        root->left = nullptr;
        root->ltag = Child;
    } else if (root->left) {
        ClearAllThreads(root->left);
    }

    // 右侧：同理
    if (root->rtag == Thread) {
        root->right = nullptr;
        root->rtag = Child;
    } else if (root->right) {
        ClearAllThreads(root->right);
    }
}

/* 静态：整棵子树刷新 parent 指针
 * 只对“孩子方向”为 Child 的分支递归；线索不参与 parent 链接。
 */
void ThreadedNode::ResetParent(ThreadedNode* root, ThreadedNode* parent)
{
    if (!root) return;
    root->parent = parent;

    if (root->ltag == Child && root->left)
        ResetParent(root->left, root);
    if (root->rtag == Child && root->right)
        ResetParent(root->right, root);
}

ThreadedNode* ThreadedNode::firstInorder()
{
    ThreadedNode* p = this;
    while (p && p->ltag == Child && p->left)
        p = p->left;
    return p;
}

ThreadedNode* ThreadedNode::inorderSuccessor()
{
    // 若右侧为线索，则 right 即后继
    if (rtag == Thread) return right;

    // 否则进入右子树，取其最左
    if (!hasRightChild()) return nullptr;
    ThreadedNode* p = right;
    while (p->ltag == Child && p->left)
        p = p->left;
    return p;
}

ThreadedNode* ThreadedNode::inorderPredecessor()
{
    // 若左侧为线索，则 left 即前驱
    if (ltag == Thread) return left;

    // 否则进入左子树，取其最右
    if (!hasLeftChild()) return nullptr;
    ThreadedNode* p = left;
    while (p->rtag == Child && p->right)
        p = p->right;
    return p;
}
