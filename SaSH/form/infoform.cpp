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

InfoForm::InfoForm(qint64 index, qint64 defaultPage, QWidget* parent)
	: QWidget(parent)
	, Indexer(index)
{
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_StyledBackground, true);

	setStyleSheet(R"(background-color: #F9F9F9)");

	Qt::WindowFlags windowflag = this->windowFlags();
	windowflag |= Qt::WindowType::Tool;
	setWindowFlag(Qt::WindowType::Tool);

	connect(this, &InfoForm::resetControlTextLanguage, this, &InfoForm::onResetControlTextLanguage, Qt::QueuedConnection);

	ui.tabWidget->clear();
	util::setTab(ui.tabWidget);

	pBattleInfoForm_ = new BattleInfoForm(index, nullptr);
	Q_ASSERT(pBattleInfoForm_ != nullptr);
	if (pBattleInfoForm_ != nullptr)
	{
		ui.tabWidget->addTab(pBattleInfoForm_, tr("battleinfo"));
	}

	pCharInfoForm_ = new CharInfoForm(index, nullptr);
	Q_ASSERT(pCharInfoForm_ != nullptr);
	if (pCharInfoForm_ != nullptr)
	{
		ui.tabWidget->addTab(pCharInfoForm_, tr("playerinfo"));
	}

	pItemInfoForm_ = new ItemInfoForm(index, nullptr);
	Q_ASSERT(pItemInfoForm_ != nullptr);
	if (pItemInfoForm_ != nullptr)
	{
		ui.tabWidget->addTab(pItemInfoForm_, tr("iteminfo"));
	}

	pChatInfoForm_ = new ChatInfoForm(index, nullptr);
	Q_ASSERT(pChatInfoForm_ != nullptr);
	if (pChatInfoForm_ != nullptr)
	{
		ui.tabWidget->addTab(pChatInfoForm_, tr("chatinfo"));
	}

	//pMailInfoForm_ = new MailInfoForm(index, nullptr);
	//Q_ASSERT(pMailInfoForm_ != nullptr);
	//if (pMailInfoForm_ != nullptr)
	//{
	//	ui.tabWidget->addTab(pMailInfoForm_, tr("mailinfo"));
	//}

	//pPetInfoForm_ = new PetInfoForm(index, nullptr);
	//Q_ASSERT(pPetInfoForm_ != nullptr);
	//if (pPetInfoForm_ != nullptr)
	//{
	//	ui.tabWidget->addTab(pPetInfoForm_, tr("petinfo"));
	//}

	pAfkInfoForm_ = new AfkInfoForm(index, nullptr);
	Q_ASSERT(pAfkInfoForm_ != nullptr);
	if (pAfkInfoForm_ != nullptr)
	{
		ui.tabWidget->addTab(pAfkInfoForm_, tr("afkinfo"));
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);

	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &InfoForm::onApplyHashSettingsToUI, Qt::QueuedConnection);

	onResetControlTextLanguage();

	util::FormSettingManager formManager(this);
	formManager.loadSettings();

	onApplyHashSettingsToUI();

	setCurrentPage(defaultPage);
}

void InfoForm::setCurrentPage(qint64 defaultPage)
{
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
	qint64 currentIndex = getIndex();
	QString title = tr("InfoForm");
	QString newTitle = QString("[%1] %2").arg(currentIndex).arg(title);
	setWindowTitle(newTitle);

	//reset tab text
	qint64 n = 0;
	ui.tabWidget->setTabText(n++, tr("battleinfo"));
	ui.tabWidget->setTabText(n++, tr("playerinfo"));
	ui.tabWidget->setTabText(n++, tr("iteminfo"));
	ui.tabWidget->setTabText(n++, tr("chatinfo"));
	//ui.tabWidget->setTabText(n++, tr("mailinfo"));
	//ui.tabWidget->setTabText(n++, tr("petinfo"));
	ui.tabWidget->setTabText(n++, tr("afkinfo"));

	pCharInfoForm_->onResetControlTextLanguage();
	pItemInfoForm_->onResetControlTextLanguage();
	pChatInfoForm_->onResetControlTextLanguage();
}

void InfoForm::onApplyHashSettingsToUI()
{
	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (!injector.server.isNull() && injector.server->getOnlineFlag())
	{
		QString title = tr("InfoForm");
		QString newTitle = QString("[%1][%2] %3").arg(currentIndex).arg(injector.server->getPC().name).arg(title);
		setWindowTitle(newTitle);
	}
}