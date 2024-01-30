#include "stdafx.h"
#include "battlesettingfrom.h"

#include "util.h"
#include "signaldispatcher.h"
#include <gamedevice.h>

BattleSettingFrom::BattleSettingFrom(long long index, QWidget* parent)
	: QWidget(parent)
	, Indexer(index)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_QuitOnClose);
	QFont font = util::getFont();
	setFont(font);

	QStringList nameCheckList;
	QList<PushButton*> buttonList = util::findWidgets<PushButton>(this);
	for (auto& button : buttonList)
	{
		if (button && !nameCheckList.contains(button->objectName()))
		{
			util::setPushButton(button);
			nameCheckList.append(button->objectName());
			connect(button, &PushButton::clicked, this, &BattleSettingFrom::onButtonClicked, Qt::UniqueConnection);
		}
	}

	QList <QCheckBox*> checkBoxList = util::findWidgets<QCheckBox>(this);
	for (auto& checkBox : checkBoxList)
	{
		if (checkBox && !nameCheckList.contains(checkBox->objectName()))
		{
			util::setCheckBox(checkBox);
			nameCheckList.append(checkBox->objectName());
			//connect(checkBox, &QCheckBox::stateChanged, this, &AfkForm::onCheckBoxStateChanged, Qt::UniqueConnection);
		}
	}

	QList <QSpinBox*> spinBoxList = util::findWidgets<QSpinBox>(this);
	for (auto& spinBox : spinBoxList)
	{
		if (spinBox && !nameCheckList.contains(spinBox->objectName()))
		{
			util::setSpinBox(spinBox);
			nameCheckList.append(spinBox->objectName());
			//connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(onSpinBoxValueChanged(int)), Qt::UniqueConnection);
		}
	}

	QList <QLineEdit*> lineEditList = util::findWidgets<QLineEdit>(this);
	for (auto& lineEdit : lineEditList)
	{
		if (lineEdit && !nameCheckList.contains(lineEdit->objectName()))
		{
			util::setLineEdit(lineEdit);
			nameCheckList.append(lineEdit->objectName());
			//connect(lineEdit, &QLineEdit::textChanged, this, &AfkForm::onLineEditTextChanged, Qt::UniqueConnection);
		}
	}

	QList <ComboBox*> comboBoxList = util::findWidgets<ComboBox>(this);
	for (auto& comboBox : comboBoxList)
	{
		if (comboBox && !nameCheckList.contains(comboBox->objectName()))
		{
			util::setComboBox(comboBox);
			nameCheckList.append(comboBox->objectName());
			//connect(comboBox, &ComboBox::clicked, this, &AfkForm::onComboBoxClicked, Qt::UniqueConnection);
			//connect(comboBox, &ComboBox::currentTextChanged, this, &AfkForm::onComboBoxTextChanged, Qt::UniqueConnection);
			//connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBoxCurrentIndexChanged(int)), Qt::UniqueConnection);
		}
	}

	QList <QLabel*> labelList = util::findWidgets<QLabel>(this);
	for (auto& label : labelList)
	{
		if (label && !nameCheckList.contains(label->objectName()))
		{
			util::setLabel(label);
			nameCheckList.append(label->objectName());
		}
	}

	QList <TableWidget*> tableList = util::findWidgets<TableWidget>(this);
	for (auto& table : tableList)
	{
		if (table && !nameCheckList.contains(table->objectName()))
		{
			util::setTableWidget(table);
			nameCheckList.append(table->objectName());
		}
	}

	QStringList conditions = {
		tr("char_hp"), tr("char_mp"), tr("char_hp%"), tr("char_mp%"), tr("char_status"), tr("char_lv"),

		tr("pet_hp"), tr("pet_hp%"), tr("pet_status"), tr("pet_lv"),

		tr("allies_hp_lowest"), tr("allies_hp%_lowest"), tr("allies_hp_highest"), tr("allies_hp%_highest"),
		tr("allies_lv_lowest"), tr("allies_lv_highest"),
		tr("allies_status"), tr("allies_count"),

		tr("enemy_hp_lowest"), tr("enemy_hp%_lowest"), tr("enemy_hp_highest"), tr("enemy_hp%_highest"),
		tr("enemy_lv_lowest"), tr("enemy_lv_highest"),
		tr("enemy_status"), tr("enemy_count"),

		tr("battle_round"),
	};

	QStringList logicList = { "==", "!=", "<", ">", "<=", ">=", tr("contains"), tr("not contains") };

	QStringList target = {
		tr("char_only"), tr("pet_only"),

		tr("allies_lowest_hp"), tr("allies_highest_hp"), tr("allies_lowest_lv"), tr("allies_highest_lv"),
		tr("allies_lowest_hp%"), tr("allies_highest_hp%"),

		tr("enemy_lowest_hp"), tr("enemy_highest_hp"), tr("enemy_lowest_lv"), tr("enemy_highest_lv"),
		tr("enemy_lowest_hp%"), tr("enemy_highest_hp%"),

	};

	ui.comboBox_conditions->addItems(conditions);
	ui.comboBox_logics->addItems(logicList);
	ui.comboBox_target->addItems(target);

	//header: conditions action target

	QStringList tableHeaders = { tr("type"), tr("conditions"), tr("action"), tr("target") };
	ui.tableWidget_logics->setColumnCount(tableHeaders.size());
	ui.tableWidget_logics->setHorizontalHeaderLabels(tableHeaders);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	connect(&signalDispatcher, &SignalDispatcher::updateComboBoxItemText, this, &BattleSettingFrom::onUpdateComboBoxItemText, Qt::QueuedConnection);
}

BattleSettingFrom::~BattleSettingFrom()
{

}

void BattleSettingFrom::onButtonClicked()
{
	PushButton* pPushButton = qobject_cast<PushButton*>(sender());
	if (!pPushButton)
		return;

	QString name = pPushButton->objectName();
	if (name.isEmpty())
		return;

	if (name == "pushButton_default")
	{
		long long row = ui.tableWidget_logics->rowCount();
		ui.tableWidget_logics->insertRow(row);
		ui.tableWidget_logics->setText(row, 0, tr("char"));
		ui.tableWidget_logics->setText(row, 1, "");
		ui.tableWidget_logics->setText(row, 2, tr("attack"));
		ui.tableWidget_logics->setText(row, 3, tr("enemy_lowest_hp"));

		row = ui.tableWidget_logics->rowCount();
		ui.tableWidget_logics->insertRow(row);
		ui.tableWidget_logics->setText(row, 0, tr("pet"));
		ui.tableWidget_logics->setText(row, 1, "");
		ui.tableWidget_logics->setText(row, 2, tr("attack"));
		ui.tableWidget_logics->setText(row, 3, tr("enemy_lowest_hp"));
		return;
	}

	if (name == "pushButton_conditionup")
	{
		util::SwapRowUp(ui.listWidget_conditions);
	}

	if (name == "pushButton_conditiondown")
	{
		util::SwapRowDown(ui.listWidget_conditions);
	}

	if (name == "pushButton_logicup")
	{
		util::SwapRowUp(ui.tableWidget_logics);
	}

	if (name == "pushButton_logicdown")
	{
		util::SwapRowDown(ui.tableWidget_logics);
	}

	if (name == "pushButton_addcondition")
	{
		QString condition = ui.comboBox_conditions->currentText();
		QString logic = ui.comboBox_logics->currentText();
		QString value = ui.lineEdit_cmpvalue->text();

		if (condition.isEmpty() || logic.isEmpty() || value.isEmpty())
			return;

		QString text = QString("%1,%2,%3").arg(condition).arg(logic).arg(value);
		ui.listWidget_conditions->addItem(text);
		return;
	}

	if (name == "pushButton_addcharlogic")
	{
		QStringList conditions;
		for (long long i = 0; i < ui.listWidget_conditions->count(); ++i)
		{
			QString text = ui.listWidget_conditions->item(i)->text();
			conditions.append(text);
		}

		conditions.removeDuplicates();

		QString conditionsStr = conditions.join("&&");
		QString action = ui.comboBox_charactions->currentText();
		if (action.contains(tr("item")))
		{
			QString item = ui.lineEdit_items->text();
			action.append(QString(":%1").arg(item));
		}

		QString target = ui.comboBox_target->currentText();

		if (conditionsStr.isEmpty() || action.isEmpty() || target.isEmpty())
			return;

		QStringList logic = QStringList{ tr("char"), conditionsStr, action, target };

		//col: 1.conditions 2.action 3.target
		long long row = ui.tableWidget_logics->rowCount();
		ui.tableWidget_logics->insertRow(row);
		for (long long i = 0; i < logic.size(); ++i)
		{
			ui.tableWidget_logics->setText(row, i, logic[i]);
		}
		return;
	}

	if (name == "pushButton_addpetlogic")
	{
		QStringList conditions;
		for (long long i = 0; i < ui.listWidget_conditions->count(); ++i)
		{
			QString text = ui.listWidget_conditions->item(i)->text();
			conditions.append(text);
		}

		conditions.removeDuplicates();

		QString conditionsStr = conditions.join("&&");
		QString action = ui.comboBox_charactions->currentText();
		QString target = ui.comboBox_target->currentText();

		if (conditionsStr.isEmpty() || action.isEmpty() || target.isEmpty())
			return;

		QStringList logic = QStringList{ tr("pet"), conditionsStr, action, target };

		//col: 1.conditions 2.action 3.target
		long long row = ui.tableWidget_logics->rowCount();
		ui.tableWidget_logics->insertRow(row);
		for (long long i = 0; i < logic.size(); ++i)
		{
			ui.tableWidget_logics->setText(row, i, logic[i]);
		}
	}
}

void BattleSettingFrom::onUpdateComboBoxItemText(long long type, const QStringList& textList)
{
	if (isHidden())
		return;

	long long currentIndex = getIndex();

	switch (type)
	{
	case util::kComboBoxCharAction:
	{
		const QStringList normalActionList = {
			tr("attack"), tr("defense"), tr("escape"), tr("item")
		};

		const QStringList equipActionList = {
			tr("head"), tr("body"), tr("righthand"), tr("leftacc"),
			tr("rightacc"), tr("belt"), tr("lefthand"), tr("shoes"),
			tr("gloves")
		};

		auto appendMagicText = [this, &normalActionList, &equipActionList, &textList, currentIndex](QComboBox* combo)->void
			{
				if (combo == nullptr)
					return;

				if (combo->hasFocus())
					return;

				if (combo->view()->hasFocus())
					return;

				combo->blockSignals(true);
				combo->setUpdatesEnabled(false);
				long long nOriginalIndex = combo->currentIndex();

				const long long base = normalActionList.size(); //基礎3個動作
				long long size = sa::MAX_MAGIC + base + sa::MAX_PROFESSION_SKILL;
				long long n = 0;

				//去除多餘的
				if (combo->count() > size)
				{
					for (long long i = static_cast<long long>(combo->count()) - 1; i >= size; --i)
						combo->removeItem(i);
				}

				//更新精靈
				for (long long i = 0; i < size; ++i)
				{
					//缺則補
					if (i >= combo->count())
						combo->addItem("");

					//重設基礎動作
					QString text = "";
					if (i < base)
					{
						text = QString("%1:%2").arg(i + 1).arg(normalActionList.value(i));
					}
					else if (i >= base && i < sa::MAX_MAGIC + 3)
					{
						text = QString("%1:%2:%3").arg(i + 1).arg(equipActionList.value(i - base)).arg(textList.value(i - base));
					}
					else
					{
						text = QString("%1:%3").arg(i + 1).arg(textList.value(i - base));
					}

					combo->setItemText(i, text);
					combo->setItemData(i, text, Qt::ToolTipRole);
					++n;
				}

				combo->setCurrentIndex(nOriginalIndex);
				combo->setUpdatesEnabled(true);
				combo->blockSignals(false);
			};

		//battle
		appendMagicText(ui.comboBox_charactions);
		break;
	}
	case util::kComboBoxPetAction:
	{
		auto appendText = [&textList](QComboBox* combo, long long max, bool noIndex)->void
			{
				if (combo == nullptr)
					return;

				if (combo->hasFocus())
					return;

				if (combo->view()->hasFocus())
					return;

				combo->blockSignals(true);
				combo->setUpdatesEnabled(false);
				long long nOriginalIndex = combo->currentIndex();

				long long size = textList.size();
				long long n = 0;

				if (combo->count() > max)
				{
					for (long long i = static_cast<long long>(combo->count()) - 1; i >= max; --i)
						combo->removeItem(i);
				}

				for (long long i = 0; i < max; ++i)
				{
					if (i >= combo->count())
						combo->addItem("");

					long long index = 0;
					QString text;
					if (!noIndex)
					{
						if (i >= size)
						{
							combo->setItemText(i, QString("%1:").arg(i + 1));
							combo->setItemData(i, QString("%1:").arg(i + 1), Qt::ToolTipRole);
							continue;
						}
						text = QString("%1:%2").arg(i + 1).arg(textList[i]);
					}
					else
					{
						if (i >= size)
							break;
						text = textList[n];
						++n;
					}

					combo->setItemText(i, text);
					index = static_cast<long long>(combo->count()) - 1;
					combo->setItemData(index, text, Qt::ToolTipRole);
				}

				combo->setCurrentIndex(nOriginalIndex);
				combo->setUpdatesEnabled(true);
				combo->blockSignals(false);
			};

		//battle
		appendText(ui.comboBox_petsctions, sa::MAX_PET_SKILL, false);

		break;
	}
	case util::kComboBoxItem:
	{

		break;
	}
	}
}
