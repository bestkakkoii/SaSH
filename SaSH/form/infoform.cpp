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



#include "signaldispatcher.h"
#include <gamedevice.h>
#include "util.h"

InfoForm::InfoForm(long long index, long long defaultPage, QWidget* parent)
	: QWidget(parent)
	, Indexer(index)
	, pBattleInfoForm_(index, nullptr)
	, pCharInfoForm_(index, nullptr)
	, pItemInfoForm_(index, nullptr)
	, pChatInfoForm_(index, nullptr)
	, pAfkInfoForm_(index, nullptr)
{
	ui.setupUi(this);
	setFont(util::getFont());

	setAttribute(Qt::WA_QuitOnClose);
	setAttribute(Qt::WA_StyledBackground, true);
	util::setWidget(this);

	Qt::WindowFlags windowflag = this->windowFlags();
	windowflag |= Qt::WindowType::Tool;
	setWindowFlag(Qt::WindowType::Tool);

	ui.tabWidget->clear();

	ui.tabWidget->addTab(&pBattleInfoForm_, tr("battleinfo"));

	ui.tabWidget->addTab(&pCharInfoForm_, tr("playerinfo"));

	ui.tabWidget->addTab(&pItemInfoForm_, tr("iteminfo"));

	ui.tabWidget->addTab(&pChatInfoForm_, tr("chatinfo"));

	//	ui.tabWidget->addTab(&pMailInfoForm_, tr("mailinfo"));

	//	ui.tabWidget->addTab(&pPetInfoForm_, tr("petinfo"));

	ui.tabWidget->addTab(&pAfkInfoForm_, tr("afkinfo"));

	util::setTab(ui.tabWidget);
	connect(ui.tabWidget, &QTabWidget::currentChanged, this, &InfoForm::onTabWidgetCurrentChanged, Qt::QueuedConnection);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);

	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &InfoForm::onApplyHashSettingsToUI, Qt::QueuedConnection);

	onResetControlTextLanguage();

	util::FormSettingManager formManager(this);
	formManager.loadSettings();

	onApplyHashSettingsToUI();

	setCurrentPage(defaultPage);

	connect(&timer, &QTimer::timeout, this, &InfoForm::updateInfo, Qt::QueuedConnection);

	hide();
	setUpdatesEnabled(false);
	blockSignals(true);
}

void InfoForm::hideEvent(QHideEvent* e)
{
	util::FormSettingManager formSettingManager(this);
	formSettingManager.saveSettings();
	QWidget::hideEvent(e);
}

void InfoForm::onTabWidgetCurrentChanged(int index)
{
	if (ui.tabWidget->currentIndex() != 0)
		timer.stop();
	else
		timer.start(10);
}

void InfoForm::updateInfo()
{
	if (ui.tabWidget->currentIndex() != 0)
		return;

	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	if (gamedevice.worker.isNull())
		return;

	gamedevice.worker->updateBattleTimeInfo();
}

void InfoForm::setCurrentPage(long long defaultPage)
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
	if (ui.tabWidget->currentIndex() == 0)
		timer.start(10);
	setUpdatesEnabled(true);
	blockSignals(false);

	util::FormSettingManager formManager(this);
	formManager.loadSettings();
	update();

	setAttribute(Qt::WA_Mapped);
	QWidget::showEvent(e);
}
void InfoForm::closeEvent(QCloseEvent* e)
{
	timer.stop();
	setUpdatesEnabled(false);
	blockSignals(true);
	util::FormSettingManager formManager(this);
	formManager.saveSettings();
	QWidget::closeEvent(e);
}

void InfoForm::onResetControlTextLanguage()
{
	//reset title
	long long currentIndex = getIndex();
	QString title = tr("InfoForm");
	QString newTitle = QString("[%1] %2").arg(currentIndex).arg(title);
	setWindowTitle(newTitle);

	//reset tab text
	long long n = 0;
	ui.tabWidget->setTabText(n++, tr("battleinfo"));
	ui.tabWidget->setTabText(n++, tr("playerinfo"));
	ui.tabWidget->setTabText(n++, tr("iteminfo"));
	ui.tabWidget->setTabText(n++, tr("chatinfo"));
	//ui.tabWidget->setTabText(n++, tr("mailinfo"));
	//ui.tabWidget->setTabText(n++, tr("petinfo"));
	ui.tabWidget->setTabText(n++, tr("afkinfo"));

	pCharInfoForm_.onResetControlTextLanguage();
	pItemInfoForm_.onResetControlTextLanguage();
	pChatInfoForm_.onResetControlTextLanguage();
}

void InfoForm::onApplyHashSettingsToUI()
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (!gamedevice.worker.isNull() && gamedevice.worker->getOnlineFlag())
	{
		QString title = tr("InfoForm");
		QString newTitle = QString("[%1][%2] %3").arg(currentIndex).arg(gamedevice.worker->getCharacter().name).arg(title);
		setWindowTitle(newTitle);
	}
}