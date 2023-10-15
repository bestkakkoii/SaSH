/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2023 All Rights Reserved.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/

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
		kItem,
	};

	SelectObjectForm(TitleType type, QWidget* parent = nullptr);

	virtual ~SelectObjectForm();

	void setRecviveList(QStringList* pList);

public slots:
	void setList(QStringList objectList);

	void setSelectList(QStringList objectList);

protected:
	virtual void showEvent(QShowEvent* e) override;

private slots:
	void onButtonClicked();

	void onAccept();

	void on_listWidget_itemDoubleClicked(QListWidgetItem* item);

	void on_listWidget_itemSelectionChanged();

private:
	void deleteItem();

	void appendItem();

private:
	Ui::SelectObjectFormClass ui;
	QList<QString>* pRecviveList_ = nullptr;
	TitleType type_;
};

static bool createSelectObjectForm(SelectObjectForm::TitleType type, const QStringList srcselectlist, const QStringList& srclist, QStringList* dst, QWidget* perent)
{
	QStringList recviveList;
	SelectObjectForm* pObjForm = q_check_ptr(new SelectObjectForm(type, perent));
	if (pObjForm != nullptr)
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