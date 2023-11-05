#include "stdafx.h"
#include "dragdropwidget.h"

void DragDropWidget::onResetControlTextLanguage()
{
	enum BattleActions
	{
		kFallDownEscape,
		kLockEscape,
		kLockAttack,
		kItemRevive,
		kSelectedRound,
		kMagicHeal,
		kItemHeal,
		kSkillMp,
		kItemMp,
		kMagicRevive,
		kIntervalRound,
		kMagicPurify,
		kAutoSwitchPet,
		kCatchPet,
		kMaxAcction,
	};

	QHash<QString, BattleActions> items = {
		{ tr("FallDownEscape"), kFallDownEscape},
		{ tr("LockEscape"), kLockEscape },
		{ tr("LockAttack"), kLockAttack },
		{ tr("ItemRevive"), kItemRevive },
		{ tr("SelectedRound"), kSelectedRound },
		{ tr("MagicHeal"), kMagicHeal },
		{ tr("ItemHeal"), kItemHeal },
		{ tr("SkillMp"), kSkillMp },
		{ tr("ItemMp"), kItemMp },
		{ tr("MagicRevive"), kMagicRevive },
		{ tr("IntervalRound"), kIntervalRound },
		{ tr("MagicPurify"), kMagicPurify },
		{ tr("AutoSwitchPet"), kAutoSwitchPet },
		{ tr("CatchPet"), kCatchPet },
	};

	long long size = kMaxAcction;
	setRowCount(kMaxAcction);

	long long count = 0;
	for (long long i = 0; i < size; ++i)
	{
		QString name = items.key(static_cast<BattleActions>(i));
		if (name.isEmpty())
			continue;

		if (rowCount() <= count)
			insertRow(count);

		QTableWidgetItem* item = q_check_ptr(new QTableWidgetItem(name));
		sash_assume(item != nullptr);
		if (item == nullptr)
			continue;

		item->setData(Qt::UserRole, i);
		item->setFlags(Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		item->setSizeHint(QSize(0, 14));
		setItem(count, 0, item);
		++count;
	}
}