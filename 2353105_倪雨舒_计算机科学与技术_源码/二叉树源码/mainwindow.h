#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "buildtree.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_startButton_clicked();   // 自动槽：进入建树页面
    void on_exitButton_clicked();    // 自动槽：退出程序
    void handleReturn();             // 普通槽：接收 BuildTree 的返回信号

private:
    Ui::MainWindow *ui;
    BuildTree *buildTreeWindow = nullptr;
};
#endif // MAINWINDOW_H
