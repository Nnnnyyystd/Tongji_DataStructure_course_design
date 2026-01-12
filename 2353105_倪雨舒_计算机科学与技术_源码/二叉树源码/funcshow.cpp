#include "funcshow.h"
#include "ui_funcshow.h"
#include "tree_scene.h"
#include "threadednode.h"

#include <QDebug>
#include <QGraphicsView>
#include <QTextBrowser>
#include <QTimer>


FuncShow::FuncShow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FuncShow)
{
    ui->setupUi(this);

    scene_ = new TreeScene(this);
    ui->view->setScene(scene_);
    ui->view->setRenderHint(QPainter::Antialiasing, true);

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &FuncShow::stepAnimation);
}

FuncShow::~FuncShow()
{
    delete ui;
}



void FuncShow::abortAndReset(bool keepOverlays)
{
    if (timer_) timer_->stop();
    animOrder_.clear();
    animIdx_ = 0;

    if (scene_) {
        if (keepOverlays) scene_->clearHighlightsOnly();  // 保留箭头，只把节点刷回白色
        else              scene_->renderTree(root_);      // 重画整棵树（会清覆盖物）
    }
    if (ui->outputBox) ui->outputBox->clear();
}

/* 外部注入根节点*/
void FuncShow::setRoot(ThreadedNode* root)
{
    root_ = root;
    threadOrder_ = ThreadOrder::None;        // 切页或重建后，视为未线索化
    scene_->renderTree(root_);
    if (ui->outputBox) ui->outputBox->clear();
}

void FuncShow::on_return_lastpageButton_clicked()
{
    timer_->stop();
    scene_->clearOverlays();
    emit backToBuildTree();
    this->hide();
}

void FuncShow::on_exitButton_clicked()
{
    qApp->quit();
}

void FuncShow::collectPreorder(ThreadedNode* p, QVector<ThreadedNode*>& out)
{
    if (!p) return;
    out.push_back(p);
    if (p->ltag == ThreadedNode::Child) collectPreorder(p->left, out);
    if (p->rtag == ThreadedNode::Child) collectPreorder(p->right, out);
}
void FuncShow::collectInorder(ThreadedNode* p, QVector<ThreadedNode*>& out)
{
    if (!p) return;
    if (p->ltag == ThreadedNode::Child) collectInorder(p->left, out);
    out.push_back(p);
    if (p->rtag == ThreadedNode::Child) collectInorder(p->right, out);
}
void FuncShow::collectPostorder(ThreadedNode* p, QVector<ThreadedNode*>& out)
{
    if (!p) return;
    if (p->ltag == ThreadedNode::Child) collectPostorder(p->left, out);
    if (p->rtag == ThreadedNode::Child) collectPostorder(p->right, out);
    out.push_back(p);
}


// 统一入口：清线索 → 递归线索化
void FuncShow::makePreorderThread_()
{
    if (!root_) return;
    ThreadedNode::ClearAllThreads(root_);
    ThreadedNode* pre = nullptr;
    preorderThreadRec_(root_, pre);
    threadOrder_ = ThreadOrder::Pre;
}
void FuncShow::makeInorderThread_()
{
    if (!root_) return;
    ThreadedNode::ClearAllThreads(root_);
    ThreadedNode* pre = nullptr;
    inorderThreadRec_(root_, pre);
    threadOrder_ = ThreadOrder::In;
}
void FuncShow::makePostorderThread_()
{
    if (!root_) return;
    ThreadedNode::ClearAllThreads(root_);
    ThreadedNode* pre = nullptr;
    postorderThreadRec_(root_, pre);
    threadOrder_ = ThreadOrder::Post;
}

// —— 三种递归：在“访问当前结点 p”时补前驱/后继 ——
// 通用规则：
//   if (!p->left)  p->setLeftThread(pre);
//   if (pre && !pre->right) pre->setRightThread(p);
//   pre = p;

void FuncShow::preorderThreadRec_(ThreadedNode* p, ThreadedNode*& pre)
{
    if (!p) return;

    // 访问 p（先序）
    if (!p->left)  p->setLeftThread(pre);
    if (pre && !pre->right) pre->setRightThread(p);
    pre = p;

    if (p->ltag == ThreadedNode::Child) preorderThreadRec_(p->left,  pre);
    if (p->rtag == ThreadedNode::Child) preorderThreadRec_(p->right, pre);
}

void FuncShow::inorderThreadRec_(ThreadedNode* p, ThreadedNode*& pre)
{
    if (!p) return;

    if (p->ltag == ThreadedNode::Child) inorderThreadRec_(p->left, pre);

    // 访问 p（中序）
    if (!p->left)  p->setLeftThread(pre);
    if (pre && !pre->right) pre->setRightThread(p);
    pre = p;

    if (p->rtag == ThreadedNode::Child) inorderThreadRec_(p->right, pre);
}

void FuncShow::postorderThreadRec_(ThreadedNode* p, ThreadedNode*& pre)
{
    if (!p) return;

    if (p->ltag == ThreadedNode::Child) postorderThreadRec_(p->left, pre);
    if (p->rtag == ThreadedNode::Child) postorderThreadRec_(p->right, pre);

    // 访问 p（后序）
    if (!p->left)  p->setLeftThread(pre);
    if (pre && !pre->right) pre->setRightThread(p);
    pre = p;
}

// 线索遍历收集（非递归）
void FuncShow::collectInorderThreaded(ThreadedNode* root, QVector<ThreadedNode*>& out)
{
    if (!root) return;
    ThreadedNode* p = root->firstInorder();
    while (p) {
        out.push_back(p);
        if (p->rtag == ThreadedNode::Thread) {
            p = p->right;                        // 线索后继
        } else {
            if (!p->right) break;
            ThreadedNode* q = p->right;
            while (q && q->ltag == ThreadedNode::Child && q->left)
                q = q->left;                     // 右子树最左
            p = q;
        }
    }
}

void FuncShow::collectPreorderThreaded(ThreadedNode* root, QVector<ThreadedNode*>& out)
{
    if (!root) return;
    ThreadedNode* p = root;
    while (p) {
        out.push_back(p);
        if (p->ltag == ThreadedNode::Child && p->left) {
            p = p->left;                         // 先走左孩子
        } else {
            p = p->right;                        // 否则右指针（右孩子或线索后继）
        }
    }
}

/* 可视化辅助  */
// 旧：根据序列画“后继箭头”演示
void FuncShow::showThreadsFromOrder(const QVector<ThreadedNode*>& order, const QColor& color)
{
    if (order.isEmpty()) return;
    timer_->stop();

    scene_->renderTree(root_);                   // 会清覆盖物
    for (int i = 0; i + 1 < order.size(); ++i)
        scene_->addArrow(order[i], order[i+1], color, /*dashed=*/true);
}

// 从整棵树按 ltag/rtag 画线索（左=前驱，右=后继）
void FuncShow::drawThreadsFromTree_(ThreadedNode* root,
                                    const QColor& leftColor,
                                    const QColor& rightColor)
{
    if (!root) return;

    // 用一个递归，仅走“孩子方向”的边，避免陷入线索环
    std::function<void(ThreadedNode*)> dfs = [&](ThreadedNode* p){
        if (!p) return;

        // 左线索（前驱）
        if (p->ltag == ThreadedNode::Thread && p->left)
            scene_->addArrow(p, p->left, leftColor, /*dashed=*/true);
        // 右线索（后继）
        if (p->rtag == ThreadedNode::Thread && p->right)
            scene_->addArrow(p, p->right, rightColor, /*dashed=*/true);

        if (p->ltag == ThreadedNode::Child) dfs(p->left);
        if (p->rtag == ThreadedNode::Child) dfs(p->right);
    };

    dfs(root);
}


void FuncShow::playTraversal(const QVector<ThreadedNode*>& order,
                             int intervalMs,
                             bool preserveOverlays)
{
    if (!root_) return;
    timer_->stop();

    if (preserveOverlays) {
        scene_->clearHighlightsOnly();           // 保留箭头，只还原高亮
    } else {
        scene_->renderTree(root_);               // 重画树（清覆盖物）
    }

    animOrder_ = order;
    animIdx_ = 0;
    if (ui->outputBox) ui->outputBox->clear();
    timer_->start(intervalMs);
}

void FuncShow::stepAnimation()
{
    if (animIdx_ > 0 && animIdx_ <= animOrder_.size())
        scene_->highlightNode(animOrder_[animIdx_-1], false);  // 上一个复原

    if (animIdx_ >= animOrder_.size()) {
        timer_->stop();
        return;
    }

    auto* n = animOrder_[animIdx_];
    scene_->highlightNode(n, true, QColor("#FFD86E"));// 当前高亮

    if (ui->outputBox)
        ui->outputBox->append(QString::number(n->value));

    ++animIdx_;
}

/*  普通遍历按钮 */
void FuncShow::on_preorder_search_clicked()
{
    abortAndReset(false);                
    qDebug() << "前序遍历（普通）";
    QVector<ThreadedNode*> v; collectPreorder(root_, v);
    playTraversal(v, 450);
}
void FuncShow::on_midorder_search_clicked()
{
    abortAndReset(false);                
    qDebug() << "中序遍历（普通）";
    QVector<ThreadedNode*> v; collectInorder(root_, v);
    playTraversal(v, 450);
}
void FuncShow::on_lastorder_search_clicked()
{
    abortAndReset(false);               
    qDebug() << "后序遍历（普通）";
    QVector<ThreadedNode*> v; collectPostorder(root_, v);
    playTraversal(v, 450);
}

/*  线索化按钮（真正改树 + 双向箭头）  */
void FuncShow::on_preclue_Button_clicked()
{
    abortAndReset(false);                
    qDebug() << "先序线索化";
    makePreorderThread_();
    scene_->renderTree(root_);
    drawThreadsFromTree_(root_, QColor("#F39C12"), QColor("#1F80FF")); // 橙=前驱，蓝=后继
}

void FuncShow::on_midclue_Button_clicked()
{
    abortAndReset(false);                
    qDebug() << "中序线索化";
    makeInorderThread_();
    scene_->renderTree(root_);
    drawThreadsFromTree_(root_, QColor("#F39C12"), QColor("#1F80FF"));
}

void FuncShow::on_lastclue_Button_clicked()
{
    abortAndReset(false);                //  先停旧动画并清场景
    qDebug() << "后序线索化";
    makePostorderThread_();
    scene_->renderTree(root_);
    drawThreadsFromTree_(root_, QColor("#F39C12"), QColor("#1F80FF"));
}

/* 线索遍历按钮（动画，保留箭头） */
void FuncShow::on_preclue_search_clicked()
{
    abortAndReset(false);                //  先停旧动画并清场景
    // 需要先处于“先序线索化”状态；若不是则自动线索化
    if (threadOrder_ != ThreadOrder::Pre) makePreorderThread_();
    scene_->renderTree(root_);
    drawThreadsFromTree_(root_, QColor("#F39C12"), QColor("#1F80FF"));

    QVector<ThreadedNode*> v; collectPreorderThreaded(root_, v);
    playTraversal(v, 450, /*preserveOverlays=*/true);
}

void FuncShow::on_midclue_search_clicked()
{
    abortAndReset(false);                //  先停旧动画并清场景
    if (threadOrder_ != ThreadOrder::In) makeInorderThread_();
    scene_->renderTree(root_);
    drawThreadsFromTree_(root_, QColor("#F39C12"), QColor("#1F80FF"));

    QVector<ThreadedNode*> v; collectInorderThreaded(root_, v);
    playTraversal(v, 450, /*preserveOverlays=*/true);
}

/* 统计节点数  */
int FuncShow::countNodes(ThreadedNode* node)
{
    abortAndReset(false);                // 先停旧动画并清场景
    if (!node) return 0;
    // 注意：线索不当孩子递归（只沿 Child 方向）
    int l = (node->ltag == ThreadedNode::Child) ? countNodes(node->left)  : 0;
    int r = (node->rtag == ThreadedNode::Child) ? countNodes(node->right) : 0;
    return 1 + l + r;
}

void FuncShow::on_count_Button_clicked()
{
    abortAndReset(false);                // 先停旧动画并清场景
    if (!root_) return;
    const int total = countNodes(root_);
    if (ui->outputBox) {
        ui->outputBox->clear();
        ui->outputBox->append(QString("当前树的节点总数: %1").arg(total));
    }
}
