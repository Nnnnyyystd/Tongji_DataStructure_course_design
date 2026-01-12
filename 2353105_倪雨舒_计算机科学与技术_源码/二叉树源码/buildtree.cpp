#include "buildtree.h"
#include "ui_buildtree.h"
#include "mainwindow.h"
#include <QMessageBox>
#include <QPainter>



BuildTree::BuildTree(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::BuildTree)
{
    ui->setupUi(this);

    scene = new TreeScene(this);
    ui->treeView->setScene(scene);
    ui->treeView->setRenderHint(QPainter::Antialiasing, true);
    ui->treeView->setDragMode(QGraphicsView::ScrollHandDrag); // 可拖动画布

    connect(scene, &TreeScene::nodeClicked,
            this,  &BuildTree::handleSceneClick);
}

BuildTree::~BuildTree()
{
    delete ui;
}

void BuildTree::on_returnButton_clicked()
{
    emit returnToMain();   // 发信号给 MainWindow
    this->hide();          // 自己隐藏
}

void BuildTree::on_exitButton_clicked()
{
    qApp->quit();          // 整个程序退出
}


/*  功能展示按钮（objectName = showfButton）*/
void BuildTree::on_showfuncButton_clicked()
{
    if (!funcShowWindow) {
        funcShowWindow = new FuncShow;                 // 创建窗口
        connect(funcShowWindow, &FuncShow::backToBuildTree,
                this,           &BuildTree::handleBack); // 收到返回
    }
    funcShowWindow->setRoot(tree.root());   // 关键：两页共用同一批节点指针
    funcShowWindow->show();   // 打开功能展示
    this->hide();             // 隐藏自己
}

/*  FuncShow 发回来的返回信号 */
void BuildTree::handleBack()
{
    this->show();             // 重新显示建树界面
}

void BuildTree::on_buildButton_clicked()
{
    const int h = ui->treeheightBox->value();  // 读 SpinBox 的层高
    tree.buildFullByHeight(h);                 // 数据结构层生成满二叉树
    scene->renderTree(tree.root());            // 画到 QGraphicsView
}
//最顶层，ui界面
void BuildTree::handleSceneClick(ThreadedNode* n)
{
    if (!n) return;

    if (!n->isLeaf()) {
        QMessageBox::information(this, "提示", "这个结点不是叶子，不能删除。");
        return;
    }

    auto ret = QMessageBox::question(this, "删除叶子",
                                     QString("确认删除叶子结点 %1 ？").arg(n->value),
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        tree.removeLeaf(n);               // 修改数据
        scene->renderTree(tree.root());   // 重新渲染（简单稳妥）
    }
}
