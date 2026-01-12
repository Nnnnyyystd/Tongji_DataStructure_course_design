#include "socialgraph.h"
#include <algorithm>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>

static inline bool isCustomType(GroupType t) {
    const int base = static_cast<int>(GroupType::Custom1);
    const int val  = static_cast<int>(t);
    return val >= base && val < base + 5;
}

static inline int customIndex(GroupType t) {
    return static_cast<int>(t) - static_cast<int>(GroupType::Custom1);
}

SocialGraph::SocialGraph(QObject* parent) : QObject(parent) {}

PersonId SocialGraph::addPerson(const Person& p)
{
    Person copy = p;
    copy.id = nextPersonId_++;
    persons.insert(copy.id, copy);
    adj.insert(copy.id, {});           // 初始化空邻接
    return copy.id;
}

bool SocialGraph::updatePerson(const Person& p)
{
    if (!checkPerson(p.id)) return false;
    Person kept = persons.value(p.id);
    // 保留 groups（组织关系），只覆盖基础字段
    Person updated = p;
    updated.groups = kept.groups;
    persons[p.id] = updated;
    return true;
}

bool SocialGraph::removePerson(PersonId id)
{
    if (!checkPerson(id)) return false;

    // 1) 从所有朋友那里移除这条无向边
    if (adj.contains(id)) {
        for (PersonId f : adj.value(id))
            adj[f].remove(id);
        adj.remove(id);
    }

    // 2) 从所有组织移除，并清空空组
    if (persons.contains(id)) {
        const auto gs = persons[id].groups;   // 拷贝一份，避免遍历时修改
        for (GroupId g : gs) {
            if (groupIndex.contains(g)) {
                groupIndex[g].remove(id);
                removeGroupIfEmpty(g);
            }
        }
    }

    // 3) 清除坐标缓存
    positions.remove(id);

    // 4) 删人本体
    persons.remove(id);
    return true;
}
GroupId SocialGraph::addGroup(const Group& g)
{
    Group copy = g;
    copy.id = nextGroupId_++;
    groups.insert(copy.id, copy);
    groupIndex.insert(copy.id, {});
    return copy.id;
}

bool SocialGraph::updateGroup(const Group& g)
{
    if (!checkGroup(g.id)) return false;
    // 成员关系不变，只覆盖字段
    Group kept = groups.value(g.id);
    Group updated = g;
    groups[g.id] = updated;
    Q_UNUSED(kept);
    return true;
}

bool SocialGraph::removeGroup(GroupId id)
{
    if (!checkGroup(id)) return false;
    // 从所有成员里删除该组织
    for (PersonId p : groupIndex.value(id))
        persons[p].groups.remove(id);

    groupIndex.remove(id);
    groups.remove(id);
    return true;
}

bool SocialGraph::addFriendship(PersonId a, PersonId b)
{
    if (a == b || !checkPerson(a) || !checkPerson(b)) return false;
    adj[a].insert(b);
    adj[b].insert(a);
    return true;
}

bool SocialGraph::removeFriendship(PersonId a, PersonId b)
{
    if (!checkPerson(a) || !checkPerson(b)) return false;
    adj[a].remove(b);
    adj[b].remove(a);
    return true;
}

bool SocialGraph::addMembership(PersonId p, GroupId g)
{
    if (!checkPerson(p) || !checkGroup(g)) return false;
    persons[p].groups.insert(g);
    groupIndex[g].insert(p);
    return true;
}

bool SocialGraph::removeMembership(PersonId p, GroupId g)
{
    if (!checkPerson(p) || !checkGroup(g)) return false;

    persons[p].groups.remove(g);
    if (groupIndex.contains(g)) {
        groupIndex[g].remove(p);
        removeGroupIfEmpty(g);                //成员关系移除后，若人数为 0，删组
    }
    return true;
}
int SocialGraph::mutualFriends(PersonId a, PersonId b) const
{
    if (!checkPerson(a) || !checkPerson(b)) return 0;
    const auto& A = adj.value(a);
    const auto& B = adj.value(b);
    int count = 0;
    for (PersonId x : A) if (B.contains(x)) ++count;
    return count;
}

int SocialGraph::sharedGroups(PersonId a, PersonId b) const
{
    if (!checkPerson(a) || !checkPerson(b)) return 0;
    const auto& A = persons.value(a).groups;
    const auto& B = persons.value(b).groups;
    int count = 0;
    for (GroupId g : A) if (B.contains(g)) ++count;
    return count;
}

QVector<SocialGraph::Suggestion>
SocialGraph::potentialAcquaintances(PersonId source, int limit,
                                    double wFriends, double wGroups) const
{
    QVector<Suggestion> out;
    if (!checkPerson(source)) return out;

    const auto& friends = adj.value(source);

    // 候选：好友的好友 + 同组织成员（集合并集）
    QSet<PersonId> candidates;
    for (PersonId f : friends) {
        for (PersonId fof : adj.value(f)) {
            if (fof != source && !friends.contains(fof))
                candidates.insert(fof);
        }
    }
    for (GroupId g : persons.value(source).groups) {
        for (PersonId m : groupIndex.value(g)) {
            if (m != source && !friends.contains(m))
                candidates.insert(m);
        }
    }

    out.reserve(candidates.size());
    for (PersonId c : candidates) {
        int cf = mutualFriends(source, c);

        // ★ 修改点 1：没有共同好友则直接忽略（“同组不算数”）
        if (cf <= 0) continue;

        // ★ 修改点 2：只有在 cf>0 时才计算/计入共同群组
        int cg = sharedGroups(source, c);
        double score = wFriends * cf + wGroups * cg;

        out.push_back(Suggestion{c, cf, cg, score});
    }

    // 排序：score 降序 → commonFriends 降序 → commonGroups 降序
    std::sort(out.begin(), out.end(),
              [](const Suggestion& a, const Suggestion& b){
                  if (a.score != b.score) return a.score > b.score;
                  if (a.commonFriends != b.commonFriends) return a.commonFriends > b.commonFriends;
                  return a.commonGroups > b.commonGroups;
              });

    if (limit >= 0 && out.size() > limit)
        out.resize(limit);
    return out;
}

void SocialGraph::clear()
{
    persons.clear();
    groups.clear();
    adj.clear();
    groupIndex.clear();
    positions.clear();
    nextPersonId_ = 1;
    nextGroupId_  = 1;
}

GroupId SocialGraph::ensureGroup(const QString& name, GroupType type)
{
    // 已存在就返回
    for (auto it = groups.begin(); it != groups.end(); ++it) {
        if (it.value().name == name && it.value().type == type)
            return it.key();
    }
    // 新建
    Group g; g.id = nextGroupId_++; g.name = name; g.type = type;
    groups.insert(g.id, g);
    groupIndex.insert(g.id, {});
    return g.id;
}

void SocialGraph::rebuildGroupsFromAttributes()
{
    // 清空旧组织（不动 persons/adj/positions）
    groups.clear();
    groupIndex.clear();
    nextGroupId_ = 1;

    for (auto it = persons.begin(); it != persons.end(); ++it) {
        it.value().groups.clear();
    }

    auto addIf = [&](PersonId pid, const QString& s, GroupType t){
        const QString n = s.trimmed();
        if (n.isEmpty()) return;
        GroupId gid = ensureGroup(n, t);
        addMembership(pid, gid);
    };

    for (auto it = persons.begin(); it != persons.end(); ++it) {
        const Person& p = it.value();
        const PersonId id = p.id;

        // 固定 6 类
        addIf(id, p.primarySchool, GroupType::PrimarySchool);
        addIf(id, p.middleSchool , GroupType::MiddleSchool );
        addIf(id, p.highSchool   , GroupType::HighSchool   );
        addIf(id, p.university   , GroupType::University   );
        addIf(id, p.company      , GroupType::Company      );
        addIf(id, p.region       , GroupType::Region       );

        // 自定义 5 类
        for (int i = 0; i < 5; ++i) {
            if (!p.custom[i].trimmed().isEmpty()) {
                addIf(id, p.custom[i], static_cast<GroupType>(static_cast<int>(GroupType::Custom1) + i));
            }
        }
    }
}
bool SocialGraph::saveToFile(const QString& path) const
{
    QJsonObject root;
    root["version"] = 2; // 升个版本号，表示支持自定义类型

    // 保存自定义类型标题（可选）
    root["custom_titles"] = QJsonArray::fromStringList(customTitles_);

    // persons
    QJsonArray arrPersons;
    for (const Person& p : persons) {
        QJsonObject po;
        po["id"]            = QString::number(p.id);
        po["name"]          = p.name;
        po["region"]        = p.region;
        po["primarySchool"] = p.primarySchool;
        po["middleSchool"]  = p.middleSchool;
        po["highSchool"]    = p.highSchool;
        po["university"]    = p.university;
        po["company"]       = p.company;

        // 自定义 5 个
        QJsonArray cust;
        for (int i=0; i<5; ++i) cust.append(p.custom[i]);
        po["custom"] = cust;

        if (positions.contains(p.id)) {
            const QPointF pos = positions[p.id];
            QJsonArray a; a.append(pos.x()); a.append(pos.y());
            po["pos"] = a;
        }
        arrPersons.append(po);
    }
    root["persons"] = arrPersons;

    // friendships（无向边，避免重复：只记录 a<b）
    QJsonArray arrEdges;
    for (auto it = adj.begin(); it != adj.end(); ++it) {
        const PersonId a = it.key();
        for (PersonId b : it.value()) {
            if (a < b) {
                QJsonArray e;
                e.append(QString::number(a));
                e.append(QString::number(b));
                arrEdges.append(e);
            }
        }
    }
    root["friendships"] = arrEdges;

    // 写文件
    QFileInfo fi(path);
    QDir().mkpath(fi.dir().path());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
    return true;
}


bool SocialGraph::loadFromFile(const QString& path)
{
    QFile f(path);
    if (!f.exists()) { clear(); return false; }
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray data = f.readAll(); f.close();

    QJsonParseError err{};
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return false;

    QJsonObject root = doc.object();
    clear();

    // custom_titles（可选）
    const QJsonArray titles = root.value("custom_titles").toArray();
    for (int i=0; i<5; ++i) {
        if (i < titles.size()) customTitles_[i] = titles.at(i).toString();
    }

    // persons
    QJsonArray arrPersons = root.value("persons").toArray();
    PersonId maxId = 0;
    for (const QJsonValue& v : arrPersons) {
        QJsonObject po = v.toObject();
        Person p;
        p.id            = po.value("id").toString().toULongLong();
        p.name          = po.value("name").toString();
        p.region        = po.value("region").toString();
        p.primarySchool = po.value("primarySchool").toString();
        p.middleSchool  = po.value("middleSchool").toString();
        p.highSchool    = po.value("highSchool").toString();
        p.university    = po.value("university").toString();
        p.company       = po.value("company").toString();

        // 读取自定义 5 个（向后兼容：老文件没有 custom 字段就保持空）
        QJsonArray cust = po.value("custom").toArray();
        for (int i=0; i<5; ++i) p.custom[i] = (i < cust.size() ? cust.at(i).toString() : QString());

        // 位置
        const QJsonArray pos = po.value("pos").toArray();
        if (pos.size() == 2) positions[p.id] = QPointF(pos.at(0).toDouble(), pos.at(1).toDouble());

        persons.insert(p.id, p);
        adj.insert(p.id, {});
        if (p.id > maxId) maxId = p.id;
    }
    nextPersonId_ = maxId + 1;

    // friendships
    const QJsonArray arrEdges = root.value("friendships").toArray();
    for (const QJsonValue& v : arrEdges) {
        const QJsonArray e = v.toArray();
        if (e.size() == 2) {
            const PersonId a = e.at(0).toString().toULongLong();
            const PersonId b = e.at(1).toString().toULongLong();
            if (checkPerson(a) && checkPerson(b)) addFriendship(a, b);
        }
    }

    // 基于 6 固定 + 5 自定义字段重建组织
    rebuildGroupsFromAttributes();
    return true;
}

QVector<QPair<PersonId,PersonId>> SocialGraph::allFriendEdges() const {
    QVector<QPair<PersonId,PersonId>> es;
    for (auto it = adj.begin(); it != adj.end(); ++it) {
        PersonId a = it.key();
        for (PersonId b : it.value()) {
            if (a < b) es.push_back({a,b});
        }
    }
    return es;
}
QStringList SocialGraph::groupNames(GroupType t) const {
    QStringList names;
    for (auto it = groups.cbegin(); it != groups.cend(); ++it) {
        if (it.value().type == t) names << it.value().name;
    }
    names.removeDuplicates();
    names.sort(Qt::CaseInsensitive);
    return names;
}


GroupId SocialGraph::findOrCreateGroupByName(const QString& name, GroupType t)
{
    const QString n = name.trimmed();
    if (n.isEmpty()) return 0;

    for (auto it = groups.begin(); it != groups.end(); ++it) {
        if (it.value().type == t &&
            it.value().name.compare(n, Qt::CaseInsensitive) == 0) {
            return it.key();
        }
    }
    return ensureGroup(n, t);
}

// socialgraph.cpp
void SocialGraph::setMembershipOfType(PersonId p, GroupType t, const QString& name)
{
    if (!checkPerson(p)) return;

    // 旧组（同类最多一个）
    GroupId oldG = 0;
    for (GroupId g : persons[p].groups) {
        if (groups.contains(g) && groups[g].type == t) { oldG = g; break; }
    }

    // 若新名称为空 -> 目标为不属于任何同类群组
    const QString trimmed = name.trimmed();
    GroupId newG = 0;
    if (!trimmed.isEmpty()) {
        newG = findOrCreateGroupByName(trimmed, t);
    }

    if (oldG == newG) {
        // 仍然要把“显示字段”刷一次（避免外部只改字符串而没换组名时不同步）
        switch (t) {
        case GroupType::PrimarySchool: persons[p].primarySchool = trimmed; break;
        case GroupType::MiddleSchool : persons[p].middleSchool  = trimmed; break;
        case GroupType::HighSchool   : persons[p].highSchool    = trimmed; break;
        case GroupType::University   : persons[p].university    = trimmed; break;
        case GroupType::Company      : persons[p].company       = trimmed; break;
        case GroupType::Region       : persons[p].region        = trimmed; break;
        default: {
            const int idx = customIndex(t);
            if (idx >= 0 && idx < 5) persons[p].custom[idx] = trimmed;
        } break;
        }
        return;
    }

    // 先移除旧组（若存在）
    if (oldG) {
        persons[p].groups.remove(oldG);
        if (groupIndex.contains(oldG)) {
            groupIndex[oldG].remove(p);
            removeGroupIfEmpty(oldG);  // 清空组
        }
    }

    // 再加入新组（若非空）
    if (newG) {
        persons[p].groups.insert(newG);
        groupIndex[newG].insert(p);
    }

    // 同步“显示字段”
    switch (t) {
    case GroupType::PrimarySchool: persons[p].primarySchool = trimmed; break;
    case GroupType::MiddleSchool : persons[p].middleSchool  = trimmed; break;
    case GroupType::HighSchool   : persons[p].highSchool    = trimmed; break;
    case GroupType::University   : persons[p].university    = trimmed; break;
    case GroupType::Company      : persons[p].company       = trimmed; break;
    case GroupType::Region       : persons[p].region        = trimmed; break;
    default: {
        const int idx = customIndex(t);
        if (idx >= 0 && idx < 5) persons[p].custom[idx] = trimmed;
    } break;
    }
}


void SocialGraph::removeGroupIfEmpty(GroupId gid)
{
    auto it = groupIndex.find(gid);
    if (it == groupIndex.end() || it.value().isEmpty()) {
        groupIndex.remove(gid);
        groups.remove(gid);
    }
}

