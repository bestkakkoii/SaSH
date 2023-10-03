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
#include <script_lua/clua.h>
#include "luascriptform.h"

#include "util.h"

#include "injector.h"

#include "signaldispatcher.h"


LuaScriptForm::LuaScriptForm(qint64 index, QWidget* parent)
	: QWidget(parent)
	, Indexer(index)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_StyledBackground);

	auto setTableWidget = [](QTableWidget* tableWidget, qint64 max_row)->void
	{
		for (qint64 row = 0; row < max_row; ++row)
		{
			tableWidget->insertRow(row);
		}

		qint64 rowCount = tableWidget->rowCount();
		qint64 columnCount = tableWidget->columnCount();
		qint64 column = 0;
		for (qint64 row = 0; row < rowCount; ++row)
		{
			for (column = 0; column < columnCount; ++column)
			{
				QTableWidgetItem* item = new QTableWidgetItem("");
				if (item)
					tableWidget->setItem(row, column, item);
			}
		}
	};


	setTableWidget(ui.tableWidget_script, 8);

	QList<PushButton*> buttonList = util::findWidgets<PushButton>(this);
	for (auto& button : buttonList)
	{
		if (button)
			connect(button, &PushButton::clicked, this, &LuaScriptForm::onButtonClicked, Qt::UniqueConnection);
	}

	connect(ui.treeWidget_script->header(), &QHeaderView::sectionClicked, this, &LuaScriptForm::onScriptTreeWidgetHeaderClicked);
	connect(ui.treeWidget_script, &QTreeWidget::itemDoubleClicked, this, &LuaScriptForm::onScriptTreeWidgetDoubleClicked);

	connect(ui.tableWidget_script, &QTableWidget::itemClicked, this, &LuaScriptForm::onScriptTableWidgetClicked);
	connect(ui.tableWidget_script, &QTableWidget::currentItemChanged, this, &LuaScriptForm::onCurrentTableWidgetItemChanged);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	connect(&signalDispatcher, &SignalDispatcher::scriptLabelRowTextChanged, this, &LuaScriptForm::onScriptLabelRowTextChanged, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptContentChanged, this, &LuaScriptForm::onScriptContentChanged, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::loadFileToTable, this, &LuaScriptForm::loadFile, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::reloadScriptList, this, &LuaScriptForm::onReloadScriptList, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptStarted, this, &LuaScriptForm::onScriptStarted, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptStoped, this, &LuaScriptForm::onScriptStoped, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptPaused, this, &LuaScriptForm::onScriptPaused, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptResumed, this, &LuaScriptForm::onScriptResumed, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptBreaked, this, &LuaScriptForm::onScriptResumed, Qt::QueuedConnection);

	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &LuaScriptForm::onApplyHashSettingsToUI, Qt::UniqueConnection);
	emit signalDispatcher.reloadScriptList();

	const QString fileName(qgetenv("JSON_PATH"));
	if (fileName.isEmpty())
		return;

	if (!QFile::exists(fileName))
		return;
	util::Config config(fileName);

	Injector& injector = Injector::getInstance(index);
	injector.currentScriptFileName = config.read<QString>(objectName(), "LastModifyFile");
	if (!injector.currentScriptFileName.isEmpty() && QFile::exists(injector.currentScriptFileName))
	{
		emit signalDispatcher.loadFileToTable(injector.currentScriptFileName);
	}

	connect(ui.spinBox_speed, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &LuaScriptForm::onSpeedChanged);
}

LuaScriptForm::~LuaScriptForm()
{
}


void LuaScriptForm::onScriptStarted()
{
	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.currentScriptFileName.isEmpty())
		return;

	if (!injector.currentScriptFileName.contains(util::SCRIPT_LUA_SUFFIX_DEFAULT))
		return;

	if (!clua_.isNull())
	{
		clua_->requestInterruption();
	}

	if (clua_->isRunning())
	{
		return;
	}

	if (!injector.scriptLogModel.isNull())
		injector.scriptLogModel->clear();

	QString content;
	bool isPrivate = false;
	if (!util::readFile(injector.currentScriptFileName, &content, &isPrivate))
		return;

	clua_.reset(new CLua(currentIndex, content));
	connect(clua_.data(), &CLua::finished, this, &LuaScriptForm::onScriptFinished);
	clua_->start();

	ui.pushButton_script_start->setEnabled(false);
	ui.pushButton_script_pause->setEnabled(true);
	ui.pushButton_script_stop->setEnabled(true);
}

void LuaScriptForm::onScriptPaused()
{
	if (!clua_.isNull())
	{
		if (!clua_->isPaused())
		{
			ui.pushButton_script_pause->setText(tr("resume"));
			clua_->paused();
		}
	}
}

void LuaScriptForm::onScriptResumed()
{
	if (!clua_.isNull())
	{
		if (clua_->isPaused())
		{
			ui.pushButton_script_pause->setText(tr("pause"));
			clua_->resumed();
		}
	}
}

void LuaScriptForm::onScriptStoped()
{
	if (!clua_.isNull())
	{
		clua_->requestInterruption();
		if (clua_->isPaused())
			clua_->resumed();
	}
}

//腳本結束信號槽
void LuaScriptForm::onScriptFinished()
{
	selectedRow_ = 0;
	ui.pushButton_script_start->setText(tr("start"));
	ui.pushButton_script_pause->setText(tr("pause"));
	ui.pushButton_script_start->setEnabled(true);
	ui.pushButton_script_pause->setEnabled(false);
	ui.pushButton_script_stop->setEnabled(false);
}

void LuaScriptForm::onButtonClicked()
{
	PushButton* pPushButton = qobject_cast<PushButton*>(sender());
	if (!pPushButton)
		return;

	QString name = pPushButton->objectName();
	if (name.isEmpty())
		return;

	qint64 currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	if (name == "pushButton_script_start")
	{
		emit signalDispatcher.scriptStarted();
	}
	else if (name == "pushButton_script_pause")
	{
		if (!clua_.isNull())
		{
			if (!clua_->isPaused())
				emit signalDispatcher.scriptPaused();
			else
				emit signalDispatcher.scriptResumed();
		}

	}
	else if (name == "pushButton_script_stop")
	{
		if (!clua_.isNull())
		{
			emit signalDispatcher.scriptStoped();
		}
	}
}

//重設表格最大行數
void LuaScriptForm::resizeTableWidgetRow(qint64 max)
{
	ui.tableWidget_script->clear();
	//set header
	QStringList header;
	header << tr("command") << tr("params");
	ui.tableWidget_script->setHorizontalHeaderLabels(header);
	qint64 rowCount = ui.tableWidget_script->rowCount();
	if (rowCount < max)
	{
		for (qint64 row = rowCount; row < max; ++row)
		{
			ui.tableWidget_script->insertRow(row);
		}
	}
	else if (rowCount > max)
	{
		for (qint64 row = rowCount - 1; row >= max; --row)
		{
			ui.tableWidget_script->removeRow(row);
		}
	}
}

//設置指定單元格內容
void LuaScriptForm::setTableWidgetItem(qint64 row, qint64 col, const QString& text)
{
	QTableWidgetItem* item = ui.tableWidget_script->item(row, col);
	if (item)
	{
		item->setText(text);
		item->setToolTip(text);
	}
	else
	{
		item = new QTableWidgetItem(text);
		if (item)
		{
			item->setToolTip(text);
			ui.tableWidget_script->setItem(row, col, item);
		}
	}
}

//樹型框header點擊信號槽
void LuaScriptForm::onScriptTreeWidgetHeaderClicked(int)
{
	qint64 currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.reloadScriptList();
	//qDebug() << "onScriptTreeWidgetClicked" << logicalIndex;
}

//更新當前行號label
void LuaScriptForm::onScriptLabelRowTextChanged(qint64 row, qint64 max, bool noSelect)
{
	ui.label_row->setText(QString("%1/%2").arg(row).arg(max));
	if (!noSelect)
	{
		ui.tableWidget_script->selectRow(row - 1);
	}
}

//加載並預覽腳本
void LuaScriptForm::loadFile(const QString& fileName)
{
	if (fileName.isEmpty())
		return;
	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	QString content;
	bool isPrivate = false;
	if (!fileName.contains(util::SCRIPT_LUA_SUFFIX_DEFAULT))
		return;

	if (!util::readFile(fileName, &content, &isPrivate))
		return;

	if (clua_.isNull())
	{
		clua_.reset(new CLua(currentIndex, content));
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(content.split("\n")));
}

void LuaScriptForm::onScriptContentChanged(const QString& fileName, const QVariant& vtokens)
{
	QStringList tokens = vtokens.value<QStringList>();

	qint64 rowCount = tokens.size();

	resizeTableWidgetRow(rowCount);

	for (qint64 row = 0; row < rowCount; ++row)
	{
		setTableWidgetItem(row, 0, tokens.value(row).trimmed());
	}

	QString newFileName = fileName;
	newFileName.replace("\\", "/");

	qint64 index = newFileName.indexOf("script/");
	if (index >= 0)
	{
		QString shortPath = fileName.mid(index + 7);
		ui.label_path->setText(shortPath);
		onScriptLabelRowTextChanged(1, rowCount, false);
	}
}

void LuaScriptForm::onCurrentTableWidgetItemChanged(QTableWidgetItem*, QTableWidgetItem*)
{

}

void LuaScriptForm::onScriptTableWidgetClicked(QTableWidgetItem*)
{
}

//樹型框雙擊事件
void LuaScriptForm::onScriptTreeWidgetDoubleClicked(QTreeWidgetItem* item, int column)
{
	if (!item)
		return;

	Q_UNUSED(column);
	if (item->text(0).isEmpty())
		return;

	IS_LOADING = true;

	do
	{
		qint64 currentIndex = getIndex();
		Injector& injector = Injector::getInstance(currentIndex);
		if (injector.IS_SCRIPT_FLAG.load(std::memory_order_acquire))
			break;

		/*得到文件路徑*/
		QStringList filepath;
		QTreeWidgetItem* itemfile = item; //獲取被點擊的item
		while (itemfile != NULL)
		{
			filepath << itemfile->text(0); //獲取itemfile名稱
			itemfile = itemfile->parent(); //將itemfile指向父item
		}
		QString strpath;
		qint64 count = (filepath.size() - 1);
		for (qint64 i = count; i >= 0; i--) //QStringlist類filepath反向存著初始item的路徑
		{ //將filepath反向輸出，相應的加入’/‘
			if (filepath.value(i).isEmpty())
				continue;
			strpath += filepath.value(i);
			if (i != 0)
				strpath += "/";
		}

		strpath = util::applicationDirPath() + "/script/" + strpath;
		strpath.replace("*", "");

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.loadFileToTable(strpath);
	} while (false);

	IS_LOADING = false;
}

//重新加載腳本列表
void LuaScriptForm::onReloadScriptList()
{
	if (IS_LOADING)
		return;

	TreeWidgetItem* item = nullptr;
	QStringList newScriptList = {};
	do
	{
		item = q_check_ptr(new TreeWidgetItem);
		if (!item) break;

		util::loadAllFileLists(item, util::applicationDirPath() + "/script/", util::SCRIPT_LUA_SUFFIX_DEFAULT, ":/image/icon_lua.png", &newScriptList);

		scriptList_ = newScriptList;
		ui.treeWidget_script->setUpdatesEnabled(false);
		ui.treeWidget_script->clear();
		ui.treeWidget_script->addTopLevelItem(item);
		//展開全部第一層
		ui.treeWidget_script->topLevelItem(0)->setExpanded(true);
		//for (qint64 i = 0; i < item->childCount(); ++i)
		//{
		//	ui.treeWidget_script->expandItem(item->child(i));
		//}

		ui.treeWidget_script->sortItems(0, Qt::AscendingOrder);

		ui.treeWidget_script->setUpdatesEnabled(true);
	} while (false);
}

void LuaScriptForm::onSpeedChanged(int value)
{
	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	injector.setValueHash(util::kScriptSpeedValue, value);

	emit SignalDispatcher::getInstance(currentIndex).scriptSpeedChanged(value);
}

void LuaScriptForm::onApplyHashSettingsToUI()
{
	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	QHash<util::UserSetting, bool> enableHash = injector.getEnablesHash();
	QHash<util::UserSetting, qint64> valueHash = injector.getValuesHash();
	QHash<util::UserSetting, QString> stringHash = injector.getStringsHash();

	ui.spinBox_speed->setValue(valueHash.value(util::kScriptSpeedValue));
}