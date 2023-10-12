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

#include <QWidget>
#include "ui_infoform.h"
#include <indexer.h>

class BattleInfoForm;
class CharInfoForm;
class ItemInfoForm;
class ChatInfoForm;
class MailInfoForm;
class PetInfoForm;
class AfkInfoForm;

class InfoForm : public QWidget, public Indexer
{
	Q_OBJECT

public:
	InfoForm(__int64 index, __int64 defaultPage, QWidget* parent);
	virtual ~InfoForm();
	void setCurrentPage(__int64 page);

signals:
	void resetControlTextLanguage();

public slots:
	void onResetControlTextLanguage();
	void onApplyHashSettingsToUI();

protected:
	virtual void showEvent(QShowEvent* e) override;
	virtual void closeEvent(QCloseEvent* e) override;

private:
	Ui::InfoFormClass ui;

	BattleInfoForm* pBattleInfoForm_ = nullptr;
	CharInfoForm* pCharInfoForm_ = nullptr;
	ItemInfoForm* pItemInfoForm_ = nullptr;
	ChatInfoForm* pChatInfoForm_ = nullptr;
	MailInfoForm* pMailInfoForm_ = nullptr;
	PetInfoForm* pPetInfoForm_ = nullptr;
	AfkInfoForm* pAfkInfoForm_ = nullptr;
};
