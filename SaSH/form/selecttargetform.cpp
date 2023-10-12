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

import Utility;
import Config;
#include "stdafx.h"
#include "selecttargetform.h"
#include <injector.h>

SelectTargetForm::SelectTargetForm(__int64 index, __int64 type, QString* dst, QWidget* parent)
	: QDialog(parent)
	, Indexer(index)
	, type_(type)
	, dst_(dst)
{
	ui.setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
	setWindowFlags(Qt::Tool | Qt::Dialog | Qt::WindowCloseButtonHint);
	setModal(true);
	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &SelectTargetForm::onAccept);
	connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	ui.buttonBox->button(QDialogButtonBox::Ok)->setText(tr("ok"));
	ui.buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("cancel"));

	QList <QCheckBox*> checkBoxList = util::findWidgets<QCheckBox>(this);
	for (auto& checkBox : checkBoxList)
	{
		if (checkBox)
			connect(checkBox, &QCheckBox::stateChanged, this, &SelectTargetForm::onCheckBoxStateChanged, Qt::QueuedConnection);
	}

	Injector& injector = Injector::getInstance(index);
	selectflag_ = static_cast<quint64>(injector.getValueHash(static_cast<UserSetting>(type)));
	checkControls();

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	connect(&signalDispatcher, &SignalDispatcher::updateTeamInfo, this, &SelectTargetForm::onUpdateTeamInfo, Qt::QueuedConnection);

	FormSettingManager formSettingManager(this);
	formSettingManager.loadSettings();

	const QHash<__int64, QString> title_hash = {
		//afk->battle button
		{ kBattleCharRoundActionTargetValue, tr("player specific round action") },
		{ kBattleCharCrossActionTargetValue, tr("Char alternating round action") },
		{ kBattleCharNormalActionTargetValue, tr("player normal round action") },

		{ kBattlePetRoundActionTargetValue, tr("pet specific round action") },
		{ kBattlePetCrossActionTargetValue, tr("pet alternating round action") },
		{ kBattlePetNormalActionTargetValue, tr("pet normal round action") },

		//afk->heal button
		{ kBattleMagicHealTargetValue, tr("magic healing target") },
		{ kBattleItemHealTargetValue, tr("item healing target") },
		{ kBattleMagicReviveTargetValue, tr("magic revival target") },
		{ kBattleItemReviveTargetValue, tr("item revival target") },

		{ kBattlePetHealTargetValue, tr("pet healing target") },
		{ kBattlePetPurgTargetValue, tr("pet purging target") },
		{ kBattleCharPurgTargetValue, tr("char purging target") },
	};

	setWindowTitle(title_hash.value(type_, tr("unknown")));
}

SelectTargetForm::~SelectTargetForm()
{
	FormSettingManager formSettingManager(this);
	formSettingManager.saveSettings();
}

void SelectTargetForm::showEvent(QShowEvent* e)
{
	setAttribute(Qt::WA_Mapped);
	QDialog::showEvent(e);
}

void SelectTargetForm::onAccept()
{
	if (dst_)
	{
		*dst_ = generateShortName(selectflag_);
		__int64 currentIndex = getIndex();
		Injector& injector = Injector::getInstance(currentIndex);
		injector.setValueHash(static_cast<UserSetting>(type_), selectflag_);
	}
	accept();
}

void SelectTargetForm::checkControls()
{
	ui.checkBox_self->setChecked(selectflag_ & kSelectSelf);
	ui.checkBox_pet->setChecked(selectflag_ & kSelectPet);
	ui.checkBox_any->setChecked(selectflag_ & kSelectAllieAny);
	ui.checkBox_all->setChecked(selectflag_ & kSelectAllieAll);
	ui.checkBox_enemy->setChecked(selectflag_ & kSelectEnemyAny);
	ui.checkBox_enemy_all->setChecked(selectflag_ & kSelectEnemyAll);
	ui.checkBox_enemy_front->setChecked(selectflag_ & kSelectEnemyFront);
	ui.checkBox_enemy_back->setChecked(selectflag_ & kSelectEnemyBack);
	ui.checkBox_leader->setChecked(selectflag_ & kSelectLeader);
	ui.checkBox_leader_pet->setChecked(selectflag_ & kSelectLeaderPet);
	ui.checkBox_teammate1->setChecked(selectflag_ & kSelectTeammate1);
	ui.checkBox_teammate1_pet->setChecked(selectflag_ & kSelectTeammate1Pet);
	ui.checkBox_teammate2->setChecked(selectflag_ & kSelectTeammate2);
	ui.checkBox_teammate2_pet->setChecked(selectflag_ & kSelectTeammate2Pet);
	ui.checkBox_teammate3->setChecked(selectflag_ & kSelectTeammate3);
	ui.checkBox_teammate3_pet->setChecked(selectflag_ & kSelectTeammate3Pet);
	ui.checkBox_teammate4->setChecked(selectflag_ & kSelectTeammate4);
	ui.checkBox_teammate4_pet->setChecked(selectflag_ & kSelectTeammate4Pet);
}

void SelectTargetForm::onCheckBoxStateChanged(__int64 state)
{
	QCheckBox* pCheckBox = qobject_cast<QCheckBox*>(sender());
	if (!pCheckBox)
		return;

	bool isChecked = (state == Qt::Checked);

	QString name = pCheckBox->objectName();
	if (name.isEmpty())
		return;

	quint64 tempFlg = 0u;

	if (name == "checkBox_self")
	{
		tempFlg = kSelectSelf;
	}
	else if (name == "checkBox_pet")
	{
		tempFlg = kSelectPet;
	}
	else if (name == "checkBox_any")
	{
		tempFlg = kSelectAllieAny;
	}
	else if (name == "checkBox_all")
	{
		tempFlg = kSelectAllieAll;
	}
	else if (name == "checkBox_enemy")
	{
		tempFlg = kSelectEnemyAny;
	}
	else if (name == "checkBox_enemy_all")
	{
		tempFlg = kSelectEnemyAll;
	}
	else if (name == "checkBox_enemy_front")
	{
		tempFlg = kSelectEnemyFront;
	}
	else if (name == "checkBox_enemy_back")
	{
		tempFlg = kSelectEnemyBack;
	}

	else if (name == "checkBox_leader")
	{
		tempFlg = kSelectLeader;
	}
	else if (name == "checkBox_leader_pet")
	{
		tempFlg = kSelectLeaderPet;
	}
	else if (name == "checkBox_teammate1")
	{
		tempFlg = kSelectTeammate1;
	}
	else if (name == "checkBox_teammate1_pet")
	{
		tempFlg = kSelectTeammate1Pet;
	}
	else if (name == "checkBox_teammate2")
	{
		tempFlg = kSelectTeammate2;
	}
	else if (name == "checkBox_teammate2_pet")
	{
		tempFlg = kSelectTeammate2Pet;
	}
	else if (name == "checkBox_teammate3")
	{
		tempFlg = kSelectTeammate3;
	}
	else if (name == "checkBox_teammate3_pet")
	{
		tempFlg = kSelectTeammate3Pet;
	}
	else if (name == "checkBox_teammate4")
	{
		tempFlg = kSelectTeammate4;
	}
	else if (name == "checkBox_teammate4_pet")
	{
		tempFlg = kSelectTeammate4Pet;
	}

	if (isChecked)
	{
		selectflag_ |= tempFlg;
	}
	else
	{
		selectflag_ &= ~tempFlg;
	}
}

QString SelectTargetForm::generateShortName(quint64 flg)
{
	QString shortName;
	if (flg & kSelectSelf)
	{
		shortName += tr("S");  // 己 (Self)
	}
	if (flg & kSelectPet)
	{
		shortName += tr("P");  // 寵 (Pet)
	}
	if (flg & kSelectAllieAny)
	{
		shortName += tr("ANY");  // 我任 (Any ally)
	}
	if (flg & kSelectAllieAll)
	{
		shortName += tr("ALL");  // 我全 (All allies)
	}
	if (flg & kSelectEnemyAny)
	{
		shortName += tr("EANY");  // 敵任 (Any enemy)
	}
	if (flg & kSelectEnemyAll)
	{
		shortName += tr("EALL");  // 敵全 (All enemies)
	}
	if (flg & kSelectEnemyFront)
	{
		shortName += tr("EF");  // 敵前 (Front enemy)
	}
	if (flg & kSelectEnemyBack)
	{
		shortName += tr("EB");  // 敵後 (Back enemy)
	}
	if (flg & kSelectLeader)
	{
		shortName += tr("L");  // 隊 (Leader)
	}
	if (flg & kSelectLeaderPet)
	{
		shortName += tr("LP");  // 隊寵 (Leader's pet)
	}
	if (flg & kSelectTeammate1)
	{
		shortName += tr("T1");  // 隊1 (Teammate 1)
	}
	if (flg & kSelectTeammate1Pet)
	{
		shortName += tr("T1P");  // 隊1寵 (Teammate 1's pet)
	}
	if (flg & kSelectTeammate2)
	{
		shortName += tr("T2");  // 隊2 (Teammate 2)
	}
	if (flg & kSelectTeammate2Pet)
	{
		shortName += tr("T2P");  // 隊2寵 (Teammate 2's pet)
	}
	if (flg & kSelectTeammate3)
	{
		shortName += tr("T3");  // 隊3 (Teammate 3)
	}
	if (flg & kSelectTeammate3Pet)
	{
		shortName += tr("T3P");  // 隊3寵 (Teammate 3's pet)
	}
	if (flg & kSelectTeammate4)
	{
		shortName += tr("T4");  // 隊4 (Teammate 4)
	}
	if (flg & kSelectTeammate4Pet)
	{
		shortName += tr("T4P");  // 隊4寵 (Teammate 4's pet)
	}

	return shortName;
}

void SelectTargetForm::onUpdateTeamInfo(const QStringList& strList)
{
	for (__int64 i = 0; i <= MAX_PARTY; ++i)
	{
		QString objName = QString("checkBox_teammate%1").arg(i);
		QCheckBox* label = ui.groupBox->findChild<QCheckBox*>(objName);
		if (!label)
			continue;
		if (i >= strList.size())
			break;

		label->setText(strList[i]);
	}
}