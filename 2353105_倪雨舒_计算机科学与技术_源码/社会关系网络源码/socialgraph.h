#ifndef SOCIALGRAPH_H
#define SOCIALGRAPH_H



#include <QObject>
#include <QHash>
#include <QVector>
#include <QString>
#include <QSet>
#include <QtGlobal>
#include <QPointF>
#include <QPair>


using PersonId = quint64;
using GroupId  = quint64;

struct Person {
    PersonId id = 0;
    QString  name;

    // 固定 6 类
    QString  region, primarySchool, middleSchool, highSchool, university, company;

    // 新增：最多 5 个自定义类型的“选中名称”（为空表示没选）
    QString  custom[5];

    QSet<GroupId> groups;
    bool operator==(const Person& other) const noexcept { return id == other.id; }
};

// socialgraph.h
enum class GroupType : quint8 {
    PrimarySchool,
    MiddleSchool,
    HighSchool,
    University,
    Company,
    Interest,
    Region,
    Custom1, Custom2, Custom3, Custom4, Custom5  // ← 新增最多 5 个自定义
};


struct Group
{
    GroupId   id = 0;
    QString   name;
    GroupType type = GroupType::Custom1;
    QString   desc;   // 可选：备注/说明
};

class SocialGraph : public QObject
{
    Q_OBJECT
public:
    explicit SocialGraph(QObject* parent = nullptr);

    // --- 基本增删改查 ---
    PersonId addPerson(const Person& person);          // 分配 id 后返回
    bool     updatePerson(const Person& person);       // 按 id 覆盖基础信息（不改关系）
    bool     removePerson(PersonId id);                // 同时清理关系

    GroupId  addGroup(const Group& group);
    bool     updateGroup(const Group& group);
    bool     removeGroup(GroupId id);                  // 同时移除成员关系

    // 好友（无向边）
    bool addFriendship(PersonId a, PersonId b);
    bool removeFriendship(PersonId a, PersonId b);

    // 组织成员关系
    bool addMembership(PersonId p, GroupId g);
    bool removeMembership(PersonId p, GroupId g);

    const Person* getPerson(PersonId id) const {
        auto it = persons.constFind(id);
        return it == persons.cend() ? nullptr : &it.value();   // value() 返回 const T&
    }

    const Group* getGroup(GroupId id) const {
        auto it = groups.constFind(id);
        return it == groups.cend() ? nullptr : &it.value();
    }

    // --- 查询辅助 ---
    int  mutualFriends(PersonId a, PersonId b) const;   // 共同好友数
    int  sharedGroups (PersonId a, PersonId b) const;   // 共同组织数

    struct Suggestion
    {
        PersonId person;
        int      commonFriends = 0;
        int      commonGroups  = 0;
        double   score         = 0.0;   // 可按权重计算
    };

    // 可能认识的人（非好友且非本人），按 score 降序；limit < 0 表示不截断
    QVector<Suggestion> potentialAcquaintances(PersonId source,
                                               int limit = -1,
                                               double wFriends = 1.0,
                                               double wGroups  = 1.0) const;

    // 便于 UI：取某人全部好友
    QSet<PersonId> friendsOf(PersonId id) const {
        return adj.contains(id) ? adj.value(id) : QSet<PersonId>{};
    }

    // 便于 UI：取某组织的全部成员
    QSet<PersonId> membersOf(GroupId gid) const {
        return groupIndex.contains(gid) ? groupIndex.value(gid) : QSet<PersonId>{};
    }

    void clear();
    bool saveToFile(const QString& path) const;
    bool loadFromFile(const QString& path);
    void rebuildGroupsFromAttributes();  // 仅用 5 类字段还原组织
    QList<PersonId> allPersons() const { return persons.keys(); }

    // 节点位置的存取
    void     setPosition(PersonId id, const QPointF& p) { positions[id] = p; }
    bool     hasPosition(PersonId id) const { return positions.contains(id); }
    QPointF  positionOf(PersonId id) const { return positions.value(id, QPointF()); }

    // 取全量好友边（a<b 去重）
    QVector<QPair<PersonId,PersonId>> allFriendEdges() const;
    // 列出某类型群组的名字（去重、排序后）
    // socialgraph.h  (class SocialGraph public:)
    QStringList groupNames(GroupType t) const;                  // 列出某类型已有群组名（去重/排序）
    GroupId     findOrCreateGroupByName(const QString& name,    // 按名+类型查找，找不到就创建
                                    GroupType t);
    // 新增：把成员在某个类别中的所属群组重置成 name（空=不属于）
    void setMembershipOfType(PersonId p, GroupType t, const QString& name);
    // === 新增：自定义类型标题（UI 要显示）
    int         customTypeCount() const { return 5; }
    QString     customTitle(int i) const { return customTitles_[i]; }
    void        setCustomTitle(int i, const QString& title) {
        if (i>=0 && i<5) customTitles_[i] = title.trimmed();
    }
    QStringList allCustomTitles() const { return customTitles_; }


private:
    QHash<PersonId, QPointF> positions; // 新增：节点坐标
    PersonId nextPersonId_ = 1;
    GroupId  nextGroupId_  = 1;

    QHash<PersonId, Person> persons;                   // 人节点
    QHash<GroupId,  Group>  groups;                    // 组织
    QHash<PersonId, QSet<PersonId>> adj;               // 邻接表（好友）
    QHash<GroupId,  QSet<PersonId>> groupIndex;        // 组织 -> 成员 倒排

    bool checkPerson(PersonId id) const { return persons.contains(id); }
    bool checkGroup (GroupId  id) const { return groups.contains(id); }
    GroupId ensureGroup(const QString& name, GroupType type);
    void removeGroupIfEmpty(GroupId gid);

     QStringList customTitles_ = {"", "", "", "", ""};

};

#endif // SOCIALGRAPH_H
