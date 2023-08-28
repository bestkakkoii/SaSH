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
#include "ui_selecttargetform.h"

class QShowEvent;

class SelectTargetForm : public QDialog
{
	Q_OBJECT
public:
	SelectTargetForm(int type, QString* dst, QWidget* parent = nullptr);
	virtual ~SelectTargetForm();
	static Q_REQUIRED_RESULT QString generateShortName(unsigned int flg);
protected:
	void showEvent(QShowEvent* e);

private slots:
	void onCheckBoxStateChanged(int state);
	void onAccept();
	void onUpdateTeamInfo(const QStringList& strList);
private:
	void checkControls();

private:
	Ui::SelectTargetFormClass ui;
	int type_ = 0;
	unsigned int selectflag_ = 0u;
	QString* dst_ = nullptr;
};

static bool createSelectTargetForm(int type, QString* dst, QWidget* perent)
{
	SelectTargetForm* pObjForm = new SelectTargetForm(type, dst, perent);
	if (pObjForm)
	{
		if (pObjForm->exec() == QDialog::Accepted)
		{
			return true;
		}
	}
	return false;
}