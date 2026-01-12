#ifndef FUNCSHOW_H
#define FUNCSHOW_H

#include <QWidget>
#include <QVector>
#include <QColor>

class ThreadedNode;
class TreeScene;
class QTimer;

namespace Ui { class FuncShow; }

class FuncShow : public QWidget
{
    Q_OBJECT
public:
    explicit FuncShow(QWidget *parent = nullptr);
    ~FuncShow();

    // 由 BuildTree 进入展示页前调用，保证两页展示同一棵树
    void setRoot(ThreadedNode* root);

signals:
    void backToBuildTree();

private slots:
    void on_return_lastpageButton_clicked();
    void on_exitButton_clicked();

    // 遍历动画（普通树，递归收集）
    void on_preorder_search_clicked();
    void on_midorder_search_clicked();
    void on_lastorder_search_clicked();

    // 线索化（真正改树：前驱/后继都建立）+ 画线索箭头
    void on_preclue_Button_clicked();
    void on_midclue_Button_clicked();
    void on_lastclue_Button_clicked();

    // 线索遍历（非递归，走线索；动画中保留箭头）
    void on_preclue_search_clicked();   // 先序线索遍历
    void on_midclue_search_clicked();   // 中序线索遍历

    // 统计节点数
    void on_count_Button_clicked();

    // 动画驱动
    void stepAnimation();

private:
    // 终止当前动画并复位场景；keepOverlays=true 时保留覆盖物（箭头），只清高亮
    void abortAndReset(bool keepOverlays = false);

    void collectPreorder (ThreadedNode* p, QVector<ThreadedNode*>& out);
    void collectInorder  (ThreadedNode* p, QVector<ThreadedNode*>& out);
    void collectPostorder(ThreadedNode* p, QVector<ThreadedNode*>& out);


    void makePreorderThread_();
    void makeInorderThread_();
    void makePostorderThread_();

    // 线索化递归核心（按照“访问当前结点时补前驱/后继”的统一规则）
    void preorderThreadRec_ (ThreadedNode* p, ThreadedNode*& pre);
    void inorderThreadRec_  (ThreadedNode* p, ThreadedNode*& pre);
    void postorderThreadRec_(ThreadedNode* p, ThreadedNode*& pre);

    void collectInorderThreaded (ThreadedNode* root, QVector<ThreadedNode*>& out);
    void collectPreorderThreaded(ThreadedNode* root, QVector<ThreadedNode*>& out);

    // 根据序列画“后继箭头”演示（i → i+1）；color 区分先/中/后
    void showThreadsFromOrder(const QVector<ThreadedNode*>& order, const QColor& color);

    // 从整棵树里按 ltag/rtag 画“左线索(前驱)/右线索(后继)”箭头
    void drawThreadsFromTree_(ThreadedNode* root,
                              const QColor& leftColor  = QColor("#F39C12"),  // 橙：前驱
                              const QColor& rightColor = QColor("#1F80FF")); // 蓝：后继

    // 播放某个序列的遍历动画
    void playTraversal(const QVector<ThreadedNode*>& order,
                       int intervalMs = 500,
                       bool preserveOverlays = false);

    // 工具
    int countNodes(ThreadedNode* node);

private:
    Ui::FuncShow *ui = nullptr;
    TreeScene*   scene_ = nullptr;
    ThreadedNode* root_ = nullptr;

    // 动画状态
    QTimer* timer_ = nullptr;
    QVector<ThreadedNode*> animOrder_;
    int animIdx_ = 0;

    // 线索状态（便于在遍历按钮里自动兜底）
    enum class ThreadOrder { None, Pre, In, Post };
    ThreadOrder threadOrder_ = ThreadOrder::None;
};

#endif // FUNCSHOW_H
