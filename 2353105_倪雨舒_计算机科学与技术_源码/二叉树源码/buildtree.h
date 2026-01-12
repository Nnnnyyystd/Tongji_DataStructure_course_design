#ifndef BUILDTREE_H
#define BUILDTREE_H

#include <QWidget>
#include "funcshow.h"
#include "binarytree.h"
#include "tree_scene.h"
namespace Ui {
class BuildTree;
}

class BuildTree : public QWidget
{
    Q_OBJECT

public:
    explicit BuildTree(QWidget *parent = nullptr);
    ~BuildTree();

private slots:                 
    void on_returnButton_clicked();  // 自动槽：返回主界面
    void on_exitButton_clicked();    // 自动槽：退出程序
    void on_showfuncButton_clicked();
    void handleBack();                      // 普通槽：收到返回
    void on_buildButton_clicked();          // “建立二叉树”按钮（根据你的 objectName 改）
    void handleSceneClick(ThreadedNode* n); // 点击节点

private:
    Ui::BuildTree *ui;
    FuncShow *funcShowWindow = nullptr;     // 指针
    TreeScene  *scene  = nullptr;           // 画布
    BinaryTree  tree;                       // 数据



signals:
    void returnToMain();  // 通知主界面回退
};

#endif // BUILDTREE_H
