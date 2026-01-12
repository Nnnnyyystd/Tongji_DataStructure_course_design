#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "shownetwork.h"

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
    void on_show_Button_clicked();
    void on_exit_Button_clicked();
    void handleReturn();


private:
    Ui::MainWindow *ui;
    ShowNetwork* showNetworkwindow = nullptr;
};
#endif // MAINWINDOW_H

