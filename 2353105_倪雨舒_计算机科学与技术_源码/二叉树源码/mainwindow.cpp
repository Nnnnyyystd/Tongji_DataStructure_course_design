#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QApplication>
#include "buildtree.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_startButton_clicked()
{
    if (!buildTreeWindow)
    {
        buildTreeWindow = new BuildTree;             // 创建子窗口
        connect(buildTreeWindow, &BuildTree::returnToMain,
                this, &MainWindow::handleReturn);    // 绑定返回信号
    }
    buildTreeWindow->show();   // 打开子窗口
    this->hide();              // 隐藏主界面
}


void MainWindow::on_exitButton_clicked()
{
    qApp->quit();              // 或 close();
}

void MainWindow::handleReturn()
{
    this->show();              // 主界面重新显示
}
