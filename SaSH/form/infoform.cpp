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

InfoForm::InfoForm(int defaultPage, QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_StyledBackground, true);

	setStyleSheet(R"(background-color: #F9F9F9)");

	Qt::WindowFlags windowflag = this->windowFlags();
	windowflag |= Qt::WindowType::Tool;
	setWindowFlag(Qt::WindowType::Tool);

	QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect;
	shadowEffect->setBlurRadius(10); // 設置陰影的模糊半徑，根據需要調整
	shadowEffect->setOffset(0, 1);   // 設置陰影的偏移量，根據需要調整
	shadowEffect->setColor(Qt::black); // 設置陰影的顏色，根據需要調整
	setGraphicsEffect(shadowEffect);

	connect(this, &InfoForm::resetControlTextLanguage, this, &InfoForm::onResetControlTextLanguage, Qt::UniqueConnection);

	QGridLayout* gridLayout = new QGridLayout;
	QOpenGLWidget* openGLWidget = new QOpenGLWidget;
	openGLWidget->setLayout(gridLayout);
	gridLayout->addWidget(ui.tabWidget);
	gridLayout->setMargin(0);

	ui.gridLayout->addWidget(openGLWidget);

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

	if (defaultPage > 0 && defaultPage <= ui.tabWidget->count())
	{
		ui.tabWidget->setCurrentIndex(defaultPage - 1);
	}
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
		QString newTitle = QString("[%1] %2").arg(injector.server->getPC().name).arg(title);
		setWindowTitle(newTitle);
	}
}


