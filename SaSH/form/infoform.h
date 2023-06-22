#pragma once

#include <QWidget>
#include "ui_infoform.h"

class BattleInfoForm;
class PlayerInfoForm;
class ItemInfoForm;
class ChatInfoForm;
class MailInfoForm;
class PetInfoForm;
class AfkInfoForm;

class InfoForm : public QWidget
{
	Q_OBJECT

public:
	InfoForm(QWidget* parent = nullptr);
	virtual ~InfoForm();

signals:
	void resetControlTextLanguage();

public slots:
	void onResetControlTextLanguage();
	void onApplyHashSettingsToUI();

protected:
	void showEvent(QShowEvent* e);
	void closeEvent(QCloseEvent* e);

private:
	Ui::InfoFormClass ui;

	BattleInfoForm* pBattleInfoForm_ = nullptr;
	PlayerInfoForm* pPlayerInfoForm_ = nullptr;
	ItemInfoForm* pItemInfoForm_ = nullptr;
	ChatInfoForm* pChatInfoForm_ = nullptr;
	MailInfoForm* pMailInfoForm_ = nullptr;
	PetInfoForm* pPetInfoForm_ = nullptr;
	AfkInfoForm* pAfkInfoForm_ = nullptr;
};
