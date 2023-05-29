#pragma once
#include <QDialog>
#include "ui_selectobjectform.h"

class QShowEvent;
class QCloseEvent;
class QString;

class SelectObjectForm : public QDialog
{
	Q_OBJECT

public:
	enum TitleType
	{
		kAutoDropItem,
		kLockAttack,
		kLockEscape,
		kAutoCatch,
		kAutoDropPet,
		kAutoLogOut,
		kWhiteList,
		kBlackList,
	};

	explicit SelectObjectForm(TitleType type, QWidget* parent = nullptr);
	virtual ~SelectObjectForm();
	void setRecviveList(QStringList* pList);

public slots:
	void setList(const QStringList& objectList);
	void setSelectList(const QStringList& objectList);

protected:
	void showEvent(QShowEvent* e);

private slots:
	void onButtonClicked();
	void onAccept();

private:
	void deleteItem();
	void appendItem();

private:
	Ui::SelectObjectFormClass ui;
	QList<QString>* pRecviveList_ = nullptr;
	TitleType type_;
};

static bool createSelectObjectForm(SelectObjectForm::TitleType type, const QStringList& srcselectlist, const QStringList& srclist, QStringList* dst, QWidget* perent)
{
	QStringList recviveList;
	SelectObjectForm* pObjForm = new SelectObjectForm(type, perent);
	if (pObjForm)
	{
		pObjForm->setRecviveList(&recviveList);
		pObjForm->setList(srclist);
		pObjForm->setSelectList(srcselectlist);
		if (pObjForm->exec() == QDialog::Accepted)
		{
			if (dst)
				*dst = recviveList;
			return true;
		}
	}
	return false;
}