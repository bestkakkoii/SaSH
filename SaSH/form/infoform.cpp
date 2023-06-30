#include "stdafx.h"
#include "infoform.h"
#include <util.h>

#include "battleinfoform.h"
#include "playerinfoform.h"
#include "iteminfoform.h"
#include "chatinfoform.h"
#include "mailinfoform.h"
#include "petinfoform.h"
#include "afkinfoform.h"

#include "signaldispatcher.h"
#include "injector.h"

InfoForm::InfoForm(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_StyledBackground, true);
	setAttribute(Qt::WA_StaticContents, true);

	connect(this, &InfoForm::resetControlTextLanguage, this, &InfoForm::onResetControlTextLanguage, Qt::UniqueConnection);


	ui.tabWidget->clear();
	util::setTab(ui.tabWidget);

	pBattleInfoForm_ = new BattleInfoForm;
	if (pBattleInfoForm_)
	{
		ui.tabWidget->addTab(pBattleInfoForm_, tr("battleinfo"));
	}

	pPlayerInfoForm_ = new PlayerInfoForm;
	if (pPlayerInfoForm_)
	{
		ui.tabWidget->addTab(pPlayerInfoForm_, tr("playerinfo"));
	}

	pItemInfoForm_ = new ItemInfoForm;
	if (pItemInfoForm_)
	{
		ui.tabWidget->addTab(pItemInfoForm_, tr("iteminfo"));
	}

	pChatInfoForm_ = new ChatInfoForm;
	if (pChatInfoForm_)
	{
		ui.tabWidget->addTab(pChatInfoForm_, tr("chatinfo"));
	}

	pMailInfoForm_ = new MailInfoForm;
	if (pMailInfoForm_)
	{
		ui.tabWidget->addTab(pMailInfoForm_, tr("mailinfo"));
	}

	pPetInfoForm_ = new PetInfoForm;
	if (pPetInfoForm_)
	{
		ui.tabWidget->addTab(pPetInfoForm_, tr("petinfo"));
	}

	pAfkInfoForm_ = new AfkInfoForm;
	if (pAfkInfoForm_)
	{
		ui.tabWidget->addTab(pAfkInfoForm_, tr("afkinfo"));
	}

	onResetControlTextLanguage();
	onApplyHashSettingsToUI();

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &InfoForm::onApplyHashSettingsToUI, Qt::UniqueConnection);


	util::FormSettingManager formManager(this);
	formManager.loadSettings();
}

InfoForm::~InfoForm()
{
	qDebug() << "InfoForm is destroyed!";
}

void InfoForm::showEvent(QShowEvent* e)
{
	setAttribute(Qt::WA_Mapped);
	QWidget::showEvent(e);
}
void InfoForm::closeEvent(QCloseEvent* e)
{
	util::FormSettingManager formManager(this);
	formManager.saveSettings();
	QWidget::closeEvent(e);
}

void InfoForm::onResetControlTextLanguage()
{
	//reset title
	setWindowTitle(tr("infoform"));

	//reset tab text
	ui.tabWidget->setTabText(0, tr("battleinfo"));
	ui.tabWidget->setTabText(1, tr("playerinfo"));
	ui.tabWidget->setTabText(2, tr("iteminfo"));
	ui.tabWidget->setTabText(3, tr("chatinfo"));
	ui.tabWidget->setTabText(4, tr("mailinfo"));
	ui.tabWidget->setTabText(5, tr("petinfo"));
	ui.tabWidget->setTabText(6, tr("afkinfo"));

	pPlayerInfoForm_->onResetControlTextLanguage();
	pItemInfoForm_->onResetControlTextLanguage();
	pChatInfoForm_->onResetControlTextLanguage();

}

void InfoForm::onApplyHashSettingsToUI()
{
	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull() && injector.server->getOnlineFlag())
	{
		QString title = tr("InfoForm");
		QString newTitle = QString("[%1] %2").arg(injector.server->pc.name).arg(title);
		setWindowTitle(newTitle);
	}
}


