#include "addmemberdialog.h"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include<QMessageBox>
// 两个对话框 cpp 顶部都放同样的工具函数


// 静态工具函数：填充 QComboBox
void AddMemberDialog::fillCombo(QComboBox* cb, const QStringList& items)
{
    cb->clear();
    cb->addItem(QString());              // 允许空选
    cb->addItems(items);
    cb->setEditable(true);               // 可手动输入
    cb->setInsertPolicy(QComboBox::NoInsert);
    cb->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
}



AddMemberDialog::AddMemberDialog(SocialGraph& g, QWidget* parent)
    : QDialog(parent), graph_(g)
{
    setWindowTitle(u8"添加新成员");
    resize(780, 560);

    // 左侧表单
    auto *form = new QFormLayout;
    auto *wLeft = new QWidget; wLeft->setLayout(form);

    leName_    = new QLineEdit;
    form->addRow(u8"姓名", leName_);

    cbPrimary_ = new QComboBox;  fillCombo(cbPrimary_, graph_.groupNames(GroupType::PrimarySchool));
    cbMiddle_  = new QComboBox;  fillCombo(cbMiddle_,  graph_.groupNames(GroupType::MiddleSchool));
    cbHigh_    = new QComboBox;  fillCombo(cbHigh_,    graph_.groupNames(GroupType::HighSchool));
    cbUniv_    = new QComboBox;  fillCombo(cbUniv_,    graph_.groupNames(GroupType::University));
    cbCompany_ = new QComboBox;  fillCombo(cbCompany_, graph_.groupNames(GroupType::Company));
    cbRegion_  = new QComboBox;  fillCombo(cbRegion_,  graph_.groupNames(GroupType::Region));

    form->addRow(u8"小学（单选）",   cbPrimary_);
    form->addRow(u8"中学（单选）",   cbMiddle_);
    form->addRow(u8"高中（单选）",   cbHigh_);
    form->addRow(u8"大学（单选）",   cbUniv_);
    form->addRow(u8"工作单位（单选）", cbCompany_);
    form->addRow(u8"地区（单选）",   cbRegion_);

    // 5 个自定义
    for (int i=0;i<5;++i) {
        cbCustom_[i] = new QComboBox;
        const auto t = static_cast<GroupType>(static_cast<int>(GroupType::Custom1)+i);
        fillCombo(cbCustom_[i], graph_.groupNames(t));
        form->addRow(graph_.customTitle(i) + u8"其余群组（单选）", cbCustom_[i]);   // 标题可自定义
    }

    // 右侧朋友勾选（已存在的逻辑保持）
    friendList_ = new QListWidget;
    friendList_->setSelectionMode(QAbstractItemView::NoSelection);
    friendList_->setUniformItemSizes(true);
    for (PersonId id : graph_.allPersons()) {
        const Person* p = graph_.getPerson(id);
        if (!p) continue;
        auto* it = new QListWidgetItem(p->name, friendList_);
        it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
        it->setData(Qt::UserRole, QVariant::fromValue<qulonglong>(id));
        it->setCheckState(Qt::Unchecked);
    }

    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btns, &QDialogButtonBox::accepted, this, &AddMemberDialog::onAccept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QHBoxLayout;
    layout->addWidget(wLeft, 1);
    auto *rightBox = new QVBoxLayout;
    rightBox->addWidget(new QLabel(u8"下列哪些是 TA 的好友（可多选）："));
    rightBox->addWidget(friendList_, 1);
    auto *wRight = new QWidget; wRight->setLayout(rightBox);
    layout->addWidget(wRight, 1);

    auto *mainV = new QVBoxLayout(this);
    mainV->addLayout(layout, 1);
    mainV->addWidget(btns, 0);
}



QSet<PersonId> AddMemberDialog::selectedFriends() const {
    QSet<PersonId> out;
    for (int i=0; i<friendList_->count(); ++i) {
        auto *it = friendList_->item(i);
        if (it->checkState() == Qt::Checked) {
            out.insert(static_cast<qulonglong>(it->data(Qt::UserRole).toULongLong()));
        }
    }
    return out;
}

// 点 OK：组装 Person + 字段，结束
void AddMemberDialog::onAccept()
{
    const QString name = leName_->text().trimmed();
    if (name.isEmpty()) {
        // 未填写姓名时给出提醒，并保持对话框不关闭
        QMessageBox::warning(
            this,
            u8"信息不完整",
            u8"请先填写“姓名”，再点击 OK。"
            );
        leName_->setFocus();
        return;
    }

    // …原有的组装 person_ 逻辑…
    person_ = Person{};
    person_.name          = name;
    person_.primarySchool = cbPrimary_->currentText().trimmed();
    person_.middleSchool  = cbMiddle_->currentText().trimmed();
    person_.highSchool    = cbHigh_->currentText().trimmed();
    person_.university    = cbUniv_->currentText().trimmed();
    person_.company       = cbCompany_->currentText().trimmed();
    person_.region        = cbRegion_->currentText().trimmed();
    for (int i = 0; i < 5; ++i)
        person_.custom[i] = cbCustom_[i]->currentText().trimmed();

    accept(); // 合法时关闭对话框
}
QStringList AddMemberDialog::customGroupTexts() const
{
    QStringList out;
    out.reserve(5);
    for (int i = 0; i < 5; ++i) {
        // 既支持从下拉选择也支持手动输入，因此直接取 currentText
        out << (cbCustom_[i] ? cbCustom_[i]->currentText().trimmed() : QString());
    }
    return out;
}
