#include "shownetwork.h"
#include "ui_shownetwork.h"
#include <QApplication>
#include <QtMath>
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include "nodeitem.h"
#include "edgeitem.h"
#include <QRandomGenerator>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLineF>
#include "editmemberdialog.h"
#include <QMessageBox>
#include <QTextEdit>
#include <algorithm>



static QString groupTypeName(GroupType t) {
    switch (t) {
    case GroupType::PrimarySchool: return QStringLiteral("小学");
    case GroupType::MiddleSchool:  return QStringLiteral("中学");
    case GroupType::HighSchool:    return QStringLiteral("高中");
    case GroupType::University:    return QStringLiteral("大学");
    case GroupType::Company:       return QStringLiteral("工作单位");
    case GroupType::Region:        return QStringLiteral("地区");
    case GroupType::Interest:      return QStringLiteral("兴趣");
    // ↓↓↓ 新增的 5 个“其余群组”类型
    case GroupType::Custom1:       return QStringLiteral("其余群组1");
    case GroupType::Custom2:       return QStringLiteral("其余群组2");
    case GroupType::Custom3:       return QStringLiteral("其余群组3");
    case GroupType::Custom4:       return QStringLiteral("其余群组4");
    case GroupType::Custom5:       return QStringLiteral("其余群组5");
    default:                       return QStringLiteral("其他");
    }
}




struct FoFEntry { PersonId id; int score; };
static QVector<FoFEntry> buildFoF(SocialGraph& g, PersonId center) {
    QVector<FoFEntry> out;
    if (!g.getPerson(center)) return out;

    const QSet<PersonId> friends = g.friendsOf(center);
    QHash<PersonId, int> score; // 候选 -> 共同好友数

    // 一层朋友的朋友（只传播一层）
    for (PersonId f : friends) {
        for (PersonId fof : g.friendsOf(f)) {
            if (fof == center) continue;
            if (friends.contains(fof)) continue; // 已是朋友的不算“可能认识”
            score[fof] += 1; // 共同好友 +1
        }
    }

    out.reserve(score.size());
    for (auto it = score.begin(); it != score.end(); ++it) {
        out.push_back({it.key(), it.value()});
    }
    std::sort(out.begin(), out.end(), [](const FoFEntry& a, const FoFEntry& b){
        if (a.score != b.score) return a.score > b.score;
        return a.id < b.id;
    });
    return out;
}

void ShowNetwork::refreshColorsAndInfo()
{
    if (!graph_.getPerson(current_)) {
        ui->infoBox->clear();
        return;
    }

    // 1) 分类着色（好友集合）
    const QSet<PersonId> friends = graph_.friendsOf(current_);

    // ★ 修改点 3：使用新规则的推荐结果（已确保 cf>0，并按 score 排好序）
    const auto recs = graph_.potentialAcquaintances(current_, -1 /*no limit*/, 1.0, 1.0);

    // 用推荐结果的人集合作为“可能认识的人”集合（与原 FoF 等价）
    QSet<PersonId> recSet;
    for (const auto& s : recs) recSet.insert(s.person);

    for (auto it = nodeMap_.begin(); it != nodeMap_.end(); ++it) {
        PersonId id = it.key();
        NodeItem* n = it.value();
        if (!n) continue;

        if (id == current_) {
            n->setRole(NodeItem::Role::Current);
        } else if (friends.contains(id)) {
            n->setRole(NodeItem::Role::Friend);    // 朋友：黄
        } else if (recSet.contains(id)) {
            n->setRole(NodeItem::Role::Suggest);   // 可能认识：绿（集合不变）
        } else {
            n->setRole(NodeItem::Role::Other);     // 其余：蓝
        }
    }

    // 2) 组装显示文本（给 QTextBrowser）
    const Person* p = graph_.getPerson(current_);
    QString info;
    info += QStringLiteral("【成员】%1\n").arg(p ? p->name : QString::number(current_));

    // —— 群组信息（原样保留）——
    QMap<GroupType, QStringList> groupsByType;
    if (p) {
        for (GroupId gid : p->groups) {
            if (const Group* g = graph_.getGroup(gid)) {
                groupsByType[g->type] << g->name;
            }
        }
    }
    const GroupType order[] = {
        GroupType::PrimarySchool, GroupType::MiddleSchool, GroupType::HighSchool,
        GroupType::University,    GroupType::Company,      GroupType::Region,
        GroupType::Custom1, GroupType::Custom2, GroupType::Custom3,
        GroupType::Custom4, GroupType::Custom5
    };

    for (GroupType t : order) {
        auto names = groupsByType.value(t);
        names.removeAll(QString());
        names.removeDuplicates();
        if (!names.isEmpty()) {
            info += QStringLiteral("【%1】%2\n")
                        .arg(groupTypeName(t), names.join(QStringLiteral("，")));
        }
    }

    // ★ 修改点 4：可能认识的人（按“新打分规则”排序后的 recs 直接输出）
    if (!recs.isEmpty()) {
        info += QStringLiteral("\n【可能认识的人】\n");
        for (const auto& s : recs) {
            const Person* pp = graph_.getPerson(s.person);
            const QString nm = pp ? pp->name : QString::number(s.person);

            // 关联度仍为“共同好友数”，共同群组来自 s.commonGroups（仅当 cf>0 才有意义）
            info += QStringLiteral("  · %1（关联度：%2，共同群组：%3）\n")
                        .arg(nm)
                        .arg(s.commonFriends)
                        .arg(s.commonGroups);
        }
    } else {
        info += QStringLiteral("\n【可能认识的人】无\n");
    }

    ui->infoBox->setPlainText(info);
}


ShowNetwork::ShowNetwork(QWidget *parent)
    : QWidget(parent), ui(new Ui::ShowNetwork)
{
    ui->setupUi(this);
    ui->infoBox->setReadOnly(true);
    ui->infoBox->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    ui->infoBox->setPlaceholderText(u8"点击图中的节点查看详细信息…");

    scene_ = new QGraphicsScene(this);
    scene_->setSceneRect(-600, -400, 1200, 800);
    ui->graphicsView->setScene(scene_);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing, true);

    connect(scene_, &QGraphicsScene::selectionChanged,
            this, &ShowNetwork::onSceneSelectionChanged);

    // 数据文件路径
    dataPath_ = QDir(QCoreApplication::applicationDirPath()).filePath("social_network.json");

    // 启动时加载
    graph_.loadFromFile(dataPath_);

    // 读取文件
    bool loaded = graph_.loadFromFile(dataPath_);

    //  若文件不存在或读到的人为空 —— 初始化两个人并保存
    if (!loaded || graph_.allPersons().isEmpty()) {
        Person p1; p1.name = "SYM"; p1.primarySchool = "省二"; p1.region = "吉林";
        Person p2; p2.name = "WJC"; p2.university    = "天津大学";  p2.company = "百度";

        PersonId id1 = graph_.addPerson(p1);
        PersonId id2 = graph_.addPerson(p2);
        graph_.addFriendship(id1, id2);
        graph_.rebuildGroupsFromAttributes();
        graph_.saveToFile(dataPath_);
    }

    // 选择默认中心并绘图
    auto ids = graph_.allPersons();
    if (!ids.isEmpty()) {
        current_ = ids.first();   // 选第一个为中心
        showFullNetwork();
    } else {
        scene_->clear();          // 理论上不会到这里
    }

    // 退出时保存
    connect(qApp, &QCoreApplication::aboutToQuit,
            this, &ShowNetwork::saveToDisk);
}

ShowNetwork::~ShowNetwork()
{
    delete ui;
}
void ShowNetwork::on_back_Button_clicked()
{
    emit returnToMain();
    this->hide();
}

void ShowNetwork::on_exit_Button_clicked()
{
    qApp->quit();
}

// 选中某个圆形节点后，切换视角到该人
void ShowNetwork::onSceneSelectionChanged()
{
    auto items = scene_->selectedItems();
    if (items.isEmpty()) return;
    for (QGraphicsItem* it : items) {
        if (auto* el = qgraphicsitem_cast<QGraphicsEllipseItem*>(it)) {
            PersonId id = static_cast<qulonglong>(el->data(0).toULongLong());
            if (graph_.getPerson(id)) {
                current_ = id;
                showFullNetwork();
                break;
            }
        }
    }
}

QBrush ShowNetwork::roleBrush(Role r) const
{
    switch (r) {
    case Role::Current: return QBrush(QColor(255, 99, 99));   // 红
    case Role::Known:   return QBrush(QColor(144, 238, 144)); // 绿
    case Role::Maybe:   return QBrush(QColor(135, 206, 250)); // 蓝
    }
    return QBrush(Qt::lightGray);
}

void ShowNetwork::drawLegend()
{
    // 左上角小图例
    const QPointF base(-570, -370);
    const int dy = 25;
    auto dot = [&](int row, const QColor& c, const QString& t){
        scene_->addEllipse(base.x(), base.y()+row*dy, 16, 16,
                           QPen(Qt::NoPen), QBrush(c));
        auto* tx = scene_->addSimpleText(t);
        tx->setPos(base.x()+22, base.y()+row*dy-2);
        tx->setBrush(Qt::black);
        tx->setZValue(2);
    };
    dot(0, QColor(255, 99, 99), "当前成员");
    dot(1, QColor(144, 238, 144), "好友");
    dot(2, QColor(135, 206, 250), "可能认识的人");
}

QMap<PersonId, QPointF>
ShowNetwork::radialPositions(const QPointF& c,
                             const QList<PersonId>& ring1,
                             const QList<PersonId>& ring2,
                             double r1, double r2) const
{
    QMap<PersonId, QPointF> pos;
    auto place = [&](const QList<PersonId>& ids, double r){
        int n = ids.size();
        for (int i=0;i<n;i++){
            double theta = (2*M_PI*i)/qMax(1, n);
            pos[ids[i]] = QPointF(c.x() + r*qCos(theta), c.y() + r*qSin(theta));
        }
    };
    place(ring1, r1);
    place(ring2, r2);
    return pos;
}


void ShowNetwork::addEdge(PersonId a, PersonId b)
{
    auto* na = nodeMap_.value(a, nullptr);
    auto* nb = nodeMap_.value(b, nullptr);
    if (!na || !nb) return;

    auto* e = new EdgeItem(na, nb);
    scene_->addItem(e);
}

void ShowNetwork::showFullNetwork()
{
    scene_->clear();
    nodeMap_.clear();
    edgeItems_.clear();

    const QRectF world(-550, -350, 1100, 700);
    auto* rng = QRandomGenerator::global();

    auto randomFreePos = [&](int maxTry = 80)->QPointF {
        for (int k=0; k<maxTry; ++k) {
            const qreal x = world.left() + rng->bounded(world.width());
            const qreal y = world.top()  + rng->bounded(world.height());
            const QPointF p(x, y);
            bool ok = true;
            for (NodeItem* n : nodeMap_.values()) {
                if (QLineF(n->pos(), p).length() < 90) { ok = false; break; }
            }
            if (ok) return p;
        }
        return QPointF(0,0);
    };

    // 1) 节点
    for (PersonId id : graph_.allPersons()) {
        const Person* per = graph_.getPerson(id);
        if (!per) continue;

        QPointF pos = graph_.hasPosition(id) ? graph_.positionOf(id)
                                             : randomFreePos();
        if (!graph_.hasPosition(id)) graph_.setPosition(id, pos);

        NodeItem::Role role = (id==current_) ? NodeItem::Role::Current
                                               : NodeItem::Role::Other;

        auto* n = new NodeItem(id, per->name, role);
        connect(n, &NodeItem::editRequested, this, &ShowNetwork::editMember);

        n->setPos(pos);
        scene_->addItem(n);

        connect(n, &NodeItem::clicked, this, [=](PersonId pid){
            current_ = pid;
            refreshColorsAndInfo();
        });

        connect(n, &QGraphicsObject::xChanged, this, [=]{
            graph_.setPosition(id, n->pos());
            graph_.saveToFile(dataPath_);
        });
        connect(n, &QGraphicsObject::yChanged, this, [=]{
            graph_.setPosition(id, n->pos());
            graph_.saveToFile(dataPath_);
        });

        nodeMap_.insert(id, n);
    }

    // 边（全网、无向去重）
    const auto edges = graph_.allFriendEdges();
    for (const auto& e : edges) {
        const PersonId a = e.first;
        const PersonId b = e.second;
        auto* na = nodeMap_.value(a, nullptr);
        auto* nb = nodeMap_.value(b, nullptr);
        if (na && nb) {
            auto* edge = new EdgeItem(na, nb);
            scene_->addItem(edge);
            edgeItems_.push_back(edge);
        }
    }

    // 初次进入也刷新一次颜色与说明
    refreshColorsAndInfo();
}



void ShowNetwork::saveToDisk()
{
    graph_.saveToFile(dataPath_);
}
void ShowNetwork::on_add_new_member_Button_clicked()
{
    AddMemberDialog dlg(graph_, this);
    if (dlg.exec() != QDialog::Accepted) return;

    // 取回用户输入
    Person p = dlg.person();
    // 加入图（分配 id）
    PersonId pid = graph_.addPerson(p);
    // 朋友关系
    for (PersonId fid : dlg.selectedFriends()) {
        graph_.addFriendship(pid, fid);
    }
    //按 6 类字段：查找或新建群组 + 建立成员关系
    auto join = [&](const QString& name, GroupType t) {
        const QString n = name.trimmed();
        if (n.isEmpty()) return;                          // 允许不选
        GroupId gid = graph_.findOrCreateGroupByName(n, t); // 在 SocialGraph 对外提供的新封装
        if (gid) graph_.addMembership(pid, gid);
    };
    join(p.primarySchool, GroupType::PrimarySchool);
    join(p.middleSchool,  GroupType::MiddleSchool);
    join(p.highSchool,    GroupType::HighSchool);
    join(p.university,    GroupType::University);
    join(p.company,       GroupType::Company);
    join(p.region,        GroupType::Region);
    // 6 类之后，追加 5 个“其余群组”
    const QStringList customs = dlg.customGroupTexts();   // 长度 5：自定义1~5
    if (!customs.isEmpty()) {
        if (customs.size() > 0) join(customs.at(0).trimmed(), GroupType::Custom1);
        if (customs.size() > 1) join(customs.at(1).trimmed(), GroupType::Custom2);
        if (customs.size() > 2) join(customs.at(2).trimmed(), GroupType::Custom3);
        if (customs.size() > 3) join(customs.at(3).trimmed(), GroupType::Custom4);
        if (customs.size() > 4) join(customs.at(4).trimmed(), GroupType::Custom5);
    }


    // 保存到 JSON
    graph_.saveToFile(dataPath_);

    // 以新成员为中心刷新
    current_ = pid;
    showFullNetwork();
}
void ShowNetwork::editMember(PersonId id)
{
    const Person* po = graph_.getPerson(id);
    if (!po) return;

    // 进入对话框前，记下“编辑前”的好友集合
    const QSet<PersonId> friendsBefore = graph_.friendsOf(id);

    EditMemberDialog dlg(graph_, id, this);
    if (dlg.exec() != QDialog::Accepted) return;

    //  删除成员
    if (dlg.deleted()) {
        // 若删除的是当前中心，换一个中心（可选）
        if (current_ == id) {
            auto ids = graph_.allPersons();
            ids.removeAll(id);
            current_ = ids.isEmpty() ? 0 : ids.first();
        }

        graph_.removePerson(id);          // 会清理好友与群组倒排
        graph_.saveToFile(dataPath_);
        showFullNetwork();
        return;
    }

    // 更新基础信息（姓名、六类字段）
    Person np = dlg.updatedPerson();
    np.id = id; // 保持 id 不变
    graph_.updatePerson(np);

    graph_.setMembershipOfType(id, GroupType::PrimarySchool, np.primarySchool);
    graph_.setMembershipOfType(id, GroupType::MiddleSchool,  np.middleSchool);
    graph_.setMembershipOfType(id, GroupType::HighSchool,    np.highSchool);
    graph_.setMembershipOfType(id, GroupType::University,    np.university);
    graph_.setMembershipOfType(id, GroupType::Company,       np.company);
    graph_.setMembershipOfType(id, GroupType::Region,        np.region);
    graph_.setMembershipOfType(id, GroupType::PrimarySchool, np.primarySchool);
    graph_.setMembershipOfType(id, GroupType::MiddleSchool,  np.middleSchool);
    graph_.setMembershipOfType(id, GroupType::HighSchool,    np.highSchool);
    graph_.setMembershipOfType(id, GroupType::University,    np.university);
    graph_.setMembershipOfType(id, GroupType::Company,       np.company);
    graph_.setMembershipOfType(id, GroupType::Region,        np.region);

    // 5 个“其余群组”（每类保持单选）
    {
        const QStringList customs = dlg.customGroupTexts();  // 长度 5
        graph_.setMembershipOfType(id, GroupType::Custom1, customs.value(0));
        graph_.setMembershipOfType(id, GroupType::Custom2, customs.value(1));
        graph_.setMembershipOfType(id, GroupType::Custom3, customs.value(2));
        graph_.setMembershipOfType(id, GroupType::Custom4, customs.value(3));
        graph_.setMembershipOfType(id, GroupType::Custom5, customs.value(4));
    }

    // 同步好友关系（新增/删除）
    const QSet<PersonId> friendsAfter = dlg.selectedFriends();

    // 需要添加的好友：after - before
    for (PersonId f : (friendsAfter - friendsBefore)) {
        if (f != id) graph_.addFriendship(id, f);
    }
    // 需要删除的好友：before - after
    for (PersonId f : (friendsBefore - friendsAfter)) {
        if (f != id) graph_.removeFriendship(id, f);
    }

    //保存与刷新
    graph_.saveToFile(dataPath_);
    showFullNetwork();
}

void ShowNetwork::on_check_group_Button_clicked()
{
    // 1) 收集所有出现过的群组 id（不直接访问 graph_ 的内部容器）
    QSet<GroupId> allGroups;
    for (PersonId pid : graph_.allPersons()) {
        if (const Person* p = graph_.getPerson(pid)) {
            for (GroupId gid : p->groups) allGroups.insert(gid);
        }
    }

    // 2) 做成可排序的列表：按类型顺序，其次按名称（不区分大小写）
    struct GItem { GroupId id; GroupType type; QString name; };
    QVector<GItem> items;
    items.reserve(allGroups.size());
    for (GroupId gid : allGroups) {
        if (const Group* g = graph_.getGroup(gid)) {
            items.push_back({gid, g->type, g->name});
        }
    }

    // 类型显示顺序（含 5 个“其余群组”）
    const QList<GroupType> typeOrder = {
        GroupType::PrimarySchool, GroupType::MiddleSchool, GroupType::HighSchool,
        GroupType::University, GroupType::Company, GroupType::Region,
        GroupType::Interest,
        GroupType::Custom1, GroupType::Custom2, GroupType::Custom3,
        GroupType::Custom4, GroupType::Custom5
    };

    auto typeRank = [&](GroupType t)->int {
        int idx = typeOrder.indexOf(t);
        return (idx >= 0) ? idx : 999; // 未列出的类型排在最后
    };

    std::sort(items.begin(), items.end(), [&](const GItem& a, const GItem& b){
        if (typeRank(a.type) != typeRank(b.type)) return typeRank(a.type) < typeRank(b.type);
        return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
    });

    // 3) 组装输出文本：按类型分段，组内按名称排序，并列出成员
    QString out;
    out += QStringLiteral("【已有群组（共 %1 个）】\n").arg(items.size());

    int curRank = -1;
    for (const auto& it : items) {
        int r = typeRank(it.type);
        if (r != curRank) {
            curRank = r;
            out += QStringLiteral("\n—— %1 ——\n").arg(groupTypeName(it.type));
        }

        // 成员名（按字典序）
        QStringList members;
        const QSet<PersonId> mids = graph_.membersOf(it.id);
        for (PersonId pid : mids) {
            if (const Person* p = graph_.getPerson(pid)) members << p->name;
        }
        members.sort(Qt::CaseInsensitive);

        // 每个群组一行：名称 + 成员(人数)
        if (members.isEmpty()) {
            out += QStringLiteral("  · %1（成员：0）\n").arg(it.name);
        } else {
            out += QStringLiteral("  · %1（成员：%2）%3\n")
                       .arg(it.name)
                       .arg(members.size())
                       .arg(QStringLiteral("：") + members.join(QStringLiteral("，")));
        }
    }

    if (items.isEmpty()) {
        out += QStringLiteral("\n（当前没有任何群组）\n");
    }

    // 4) 输出到左侧 QTextBrowser
    ui->infoBox->setPlainText(out);
}
