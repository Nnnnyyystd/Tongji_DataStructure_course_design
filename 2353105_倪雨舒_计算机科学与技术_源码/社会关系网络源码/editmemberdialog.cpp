#include "editmemberdialog.h"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>




// 静态工具函数：填充 QComboBox
void EditMemberDialog::fillCombo(QComboBox* cb, const QStringList& items)
{
    cb->clear();
    cb->addItem(QString());              // 允许空选
    cb->addItems(items);
    cb->setEditable(true);               // 可手动输入
    cb->setInsertPolicy(QComboBox::NoInsert);
    cb->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
}


EditMemberDialog::EditMemberDialog(SocialGraph& g, PersonId id, QWidget* parent)
    : QDialog(parent), graph_(g), id_(id)
{
    setWindowTitle(u8"编辑成员");
    resize(820, 520);

    const Person* old = graph_.getPerson(id_);
    person_ = old ? *old : Person{};

    auto *form = new QFormLayout;
    auto *wLeft = new QWidget; wLeft->setLayout(form);

    leName_ = new QLineEdit;  leName_->setText(person_.name);
    form->addRow(u8"姓名", leName_);

    cbPrimary_ = new QComboBox; fillCombo(cbPrimary_, graph_.groupNames(GroupType::PrimarySchool));
    cbMiddle_  = new QComboBox; fillCombo(cbMiddle_ , graph_.groupNames(GroupType::MiddleSchool));
    cbHigh_    = new QComboBox; fillCombo(cbHigh_   , graph_.groupNames(GroupType::HighSchool));
    cbUniv_    = new QComboBox; fillCombo(cbUniv_   , graph_.groupNames(GroupType::University));
    cbCompany_ = new QComboBox; fillCombo(cbCompany_, graph_.groupNames(GroupType::Company));
    cbRegion_  = new QComboBox; fillCombo(cbRegion_ , graph_.groupNames(GroupType::Region));

    cbPrimary_->setCurrentText(person_.primarySchool);
    cbMiddle_->setCurrentText(person_.middleSchool);
    cbHigh_->setCurrentText(person_.highSchool);
    cbUniv_->setCurrentText(person_.university);
    cbCompany_->setCurrentText(person_.company);
    cbRegion_->setCurrentText(person_.region);

    form->addRow(u8"小学（单选）",   cbPrimary_);
    form->addRow(u8"中学（单选）",   cbMiddle_);
    form->addRow(u8"高中（单选）",   cbHigh_);
    form->addRow(u8"大学（单选）",   cbUniv_);
    form->addRow(u8"工作单位（单选）", cbCompany_);
    form->addRow(u8"地区（单选）",   cbRegion_);

    // 自定义 5 个
    for (int i=0;i<5;++i) {
        cbCustom_[i] = new QComboBox;
        const auto t = static_cast<GroupType>(static_cast<int>(GroupType::Custom1)+i);
        fillCombo(cbCustom_[i], graph_.groupNames(t));
        cbCustom_[i]->setCurrentText(person_.custom[i]);
        form->addRow(graph_.customTitle(i) + u8"其余群组（单选）", cbCustom_[i]);
    }

    // 右侧：朋友勾选（把当前好友勾上）
    friendList_ = new QListWidget;
    friendList_->setSelectionMode(QAbstractItemView::NoSelection);
    friendList_->setUniformItemSizes(true);

    const QSet<PersonId> friendsNow = graph_.friendsOf(id_);
    for (PersonId pid : graph_.allPersons()) {
        if (pid == id_) continue;
        const Person* p = graph_.getPerson(pid);
        if (!p) continue;
        auto* it = new QListWidgetItem(p->name, friendList_);
        it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
        it->setData(Qt::UserRole, QVariant::fromValue<qulonglong>(pid));
        it->setCheckState(friendsNow.contains(pid) ? Qt::Checked : Qt::Unchecked);
    }

    auto *btns   = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    auto *delBtn = btns->addButton(u8"删除该成员", QDialogButtonBox::DestructiveRole);
    connect(btns,   &QDialogButtonBox::accepted, this, &EditMemberDialog::onAccept);
    connect(btns,   &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(delBtn, &QPushButton::clicked,       this, &EditMemberDialog::onDelete);

    auto *layout = new QHBoxLayout;
    layout->addWidget(wLeft, 1);
    auto *rightBox = new QVBoxLayout;
    rightBox->addWidget(new QLabel(u8"与下列成员的好友关系（可多选）："));
    rightBox->addWidget(friendList_, 1);
    auto *wRight = new QWidget; wRight->setLayout(rightBox);
    layout->addWidget(wRight, 1);

    auto *mainV = new QVBoxLayout(this);
    mainV->addLayout(layout, 1);
    mainV->addWidget(btns, 0);
}


QSet<PersonId> EditMemberDialog::selectedFriends() const
{
    QSet<PersonId> out;
    for (int i=0; i<friendList_->count(); ++i) {
        auto *it = friendList_->item(i);
        if (it->checkState() == Qt::Checked) {
            out.insert(static_cast<qulonglong>(it->data(Qt::UserRole).toULongLong()));
        }
    }
    return out;
}

void EditMemberDialog::onAccept()
{
    const QString name = leName_->text().trimmed();
    if (name.isEmpty()) { leName_->setFocus(); return; }

    person_.name          = name;
    person_.primarySchool = cbPrimary_->currentText().trimmed();
    person_.middleSchool  = cbMiddle_->currentText().trimmed();
    person_.highSchool    = cbHigh_->currentText().trimmed();
    person_.university    = cbUniv_->currentText().trimmed();
    person_.company       = cbCompany_->currentText().trimmed();
    person_.region        = cbRegion_->currentText().trimmed();

    for (int i=0;i<5;++i)
        person_.custom[i] = cbCustom_[i]->currentText().trimmed();

    // 收集勾选的好友
    selFriends_.clear();
    for (int i=0; i<friendList_->count(); ++i) {
        auto* it = friendList_->item(i);
        if (it->checkState() == Qt::Checked) {
            selFriends_.insert(static_cast<qulonglong>(it->data(Qt::UserRole).toULongLong()));
        }
    }

    accept();
}

void EditMemberDialog::onDelete()
{
    const auto ret = QMessageBox::question(
        this, u8"确认删除",
        u8"删除后将移除该成员及其所有关系，且不可恢复。是否确定？",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No
        );
    if (ret == QMessageBox::Yes) {
        deleted_ = true;
        accept();  // 返回给外层处理删除
    }
}
QStringList EditMemberDialog::customGroupTexts() const
{
    QStringList out;
    out.reserve(5);
    for (int i = 0; i < 5; ++i) {
        // 既支持从下拉选择也支持手动输入，因此直接取 currentText
        out << (cbCustom_[i] ? cbCustom_[i]->currentText().trimmed() : QString());
    }
    return out;
}
