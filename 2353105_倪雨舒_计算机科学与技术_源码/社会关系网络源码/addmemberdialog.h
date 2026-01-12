// addmemberdialog.h
#pragma once
#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include "socialgraph.h"

class AddMemberDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddMemberDialog(SocialGraph& g, QWidget* parent=nullptr);

    Person person() const { return person_; }
    QSet<PersonId> selectedFriends() const;
    QStringList customGroupTexts() const;   // 返回 5 个其余群组文本


private slots:
    void onAccept();

private:
    static void fillCombo(QComboBox* cb, const QStringList& items);

    SocialGraph& graph_;
    Person       person_;

    QLineEdit *leName_;
    QComboBox *cbPrimary_, *cbMiddle_, *cbHigh_, *cbUniv_, *cbCompany_, *cbRegion_;
    QComboBox *cbCustom_[5];                 //  新增：5 个自定义群组

    QListWidget* friendList_;
};
