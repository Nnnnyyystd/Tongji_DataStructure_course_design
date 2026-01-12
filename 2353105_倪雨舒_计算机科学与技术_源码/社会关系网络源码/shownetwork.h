#ifndef SHOWNETWORK_H
#define SHOWNETWORK_H

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>
#include <QMap>
#include "socialgraph.h"
#include "addmemberdialog.h"
#include "nodeitem.h"

class NodeItem;
class EdgeItem;
namespace Ui { class ShowNetwork; }

class ShowNetwork : public QWidget
{
    Q_OBJECT
public:
    explicit ShowNetwork(QWidget *parent = nullptr);
    ~ShowNetwork();

private slots:
    void on_back_Button_clicked();
    void on_exit_Button_clicked();
    void onSceneSelectionChanged();  // 选中某个节点后切换视角
    void on_add_new_member_Button_clicked();
    void editMember(PersonId id);
    void on_check_group_Button_clicked();

private:
    Ui::ShowNetwork *ui;

    // 数据层
    SocialGraph graph_;
    PersonId    current_{0};

    // 仅用于可视化
    QGraphicsScene* scene_{nullptr};
    QMap<PersonId, NodeItem*> nodeMap_;
    QVector<EdgeItem*> edgeItems_;
    QVector<QGraphicsLineItem*> edges_;

    // 构建/刷新
    void buildDemoData();
    void drawEgoNetwork(PersonId center, int suggestLimit = 12);

    // 绘制辅助
    enum class Role { Current, Known, Maybe };
    QBrush roleBrush(Role r) const;
    void   drawLegend();

    void   addEdge(PersonId a, PersonId b);

    // 简单环形布局：返回每个 id 的坐标
    QMap<PersonId, QPointF> radialPositions(const QPointF& center,
                                            const QList<PersonId>& ring1,
                                            const QList<PersonId>& ring2,
                                            double r1 = 220, double r2 = 330) const;
    QString dataPath_;
    void saveToDisk();          // 退出时保存
    void showFullNetwork();
    void refreshColorsAndInfo();
    void updateInfoBox(PersonId center);   // ：刷新右侧信息面板


signals:
    void returnToMain();
};

#endif // SHOWNETWORK_H
