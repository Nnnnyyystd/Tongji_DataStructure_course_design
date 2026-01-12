#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QApplication>
#include "shownetwork.h"
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


void MainWindow::on_exit_Button_clicked()
{
    qApp->quit();
}


void MainWindow::on_show_Button_clicked()
{
    //qApp->quit();
    if(!showNetworkwindow)
    {
        showNetworkwindow = new ShowNetwork;
        connect(showNetworkwindow,&ShowNetwork::returnToMain,this,&MainWindow::handleReturn);
    }
    showNetworkwindow->show();
    this->hide();
}

void MainWindow::handleReturn()
{
    this->show();              // 主界面重新显示
}


