#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //  直接内联写样式
    a.setStyleSheet(R"(
        QPushButton {
            background-color: #ecf5ff;           /* 浅蓝底 */
            color: #1f2328;                       /* 深色文字 */
            border: 1px solid #90caa1;            /* 边框 */
            border-radius: 4px;
            padding: 8px 16px;
            font: 11px "Microsoft YaHei";
        }
        QPushButton:hover {
            background-color: #bbdefb;            /* 悬停加深 */
            border-color: #64b5f6;
        }
        QPushButton:pressed {
            background-color: #64b5f6;            /* 按下更深 */
            border-color: #1e88e5;
            color: white;
        }
        QPushButton:disabled {
            background-color: #eef3f8;
            border-color: #d0d7de;
            color: #98a2ad;
        }

    )");

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "BinaryTree_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    MainWindow w;
    w.show();
    return a.exec();
}
