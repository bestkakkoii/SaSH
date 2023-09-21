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
#include <indexer.h>

class QShowEvent;

class SelectTargetForm : public QDialog, public Indexer
{
	Q_OBJECT
public:
	SelectTargetForm(qint64 index, qint64 type, QString* dst, QWidget* parent = nullptr);

	virtual ~SelectTargetForm();

	static Q_REQUIRED_RESULT QString generateShortName(quint64 flg);

protected:
	virtual void showEvent(QShowEvent* e) override;

private slots:
	void onCheckBoxStateChanged(qint64 state);

	void onAccept();

	void onUpdateTeamInfo(const QStringList& strList);

private:
	void checkControls();

private:
	Ui::SelectTargetFormClass ui;
	qint64 type_ = 0;
	quint64 selectflag_ = 0u;
	QString* dst_ = nullptr;
};

static bool createSelectTargetForm(qint64 index, qint64 type, QString* dst, QWidget* perent)
{
	SelectTargetForm* pObjForm = new SelectTargetForm(index, type, dst, perent);
	if (pObjForm)
	{
		if (pObjForm->exec() == QDialog::Accepted)
		{
			return true;
		}
	}
	return false;
}