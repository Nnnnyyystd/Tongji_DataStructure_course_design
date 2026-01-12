// editmemberdialog.h
#pragma once
#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include "socialgraph.h"

class EditMemberDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EditMemberDialog(SocialGraph& g, PersonId id, QWidget* parent=nullptr);

    Person updatedPerson() const { return person_; }
    bool   deleted() const { return deleted_; }
    QSet<PersonId> selectedFriends() const ;
    QStringList customGroupTexts() const;   // 返回 5 个其余群组文本


private slots:
    void onAccept();
    void onDelete();

private:
    static void fillCombo(QComboBox* cb, const QStringList& items);

    SocialGraph& graph_;
    PersonId     id_;
    Person       person_;

    QLineEdit *leName_;
    QComboBox *cbPrimary_, *cbMiddle_, *cbHigh_, *cbUniv_, *cbCompany_, *cbRegion_;
    QComboBox *cbCustom_[5];                         // 新增

    QListWidget* friendList_;                        // 已经有的话保留
    QSet<PersonId> selFriends_;                      // 保存界面选择的朋友
    bool deleted_ = false;
};
