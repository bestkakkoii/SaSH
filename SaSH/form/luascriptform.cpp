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


LuaScriptForm::LuaScriptForm(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_StyledBackground);

	auto setTableWidget = [](QTableWidget* tableWidget, int max_row)->void
	{
		//tablewidget set single selection
		tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
		//tablewidget set selection behavior
		tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
		//set auto resize to form size
		tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
		tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

		tableWidget->setStyleSheet(R"(
		QTableWidget { font-size:11px; } 
			QTableView::item:selected { background-color: black; color: white;
		})");
		tableWidget->verticalHeader()->setDefaultSectionSize(11);
		tableWidget->horizontalHeader()->setStretchLastSection(true);
		tableWidget->horizontalHeader()->setHighlightSections(false);
		tableWidget->verticalHeader()->setHighlightSections(false);
		tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
		tableWidget->verticalHeader()->setDefaultAlignment(Qt::AlignLeft);

		for (int row = 0; row < max_row; ++row)
		{
			tableWidget->insertRow(row);
		}

		int rowCount = tableWidget->rowCount();
		int columnCount = tableWidget->columnCount();
		for (int row = 0; row < rowCount; ++row)
		{
			for (int column = 0; column < columnCount; ++column)
			{
				QTableWidgetItem* item = new QTableWidgetItem("");
				if (item)
					tableWidget->setItem(row, column, item);
			}
		}
	};

	ui.treeWidget_script->setStyleSheet(R"(
		QTreeWidget { font-size:11px; } 
		QTreeView::item:selected { background-color: black; color: white; } 
	)");
	ui.treeWidget_script->header()->setSectionsClickable(true);
	ui.treeWidget_script->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui.treeWidget_script->resizeColumnToContents(1);
	ui.treeWidget_script->sortItems(0, Qt::AscendingOrder);

	setTableWidget(ui.tableWidget_script, 8);

	QList<QPushButton*> buttonList = util::findWidgets<QPushButton>(this);
	for (auto& button : buttonList)
	{
		if (button)
			connect(button, &QPushButton::clicked, this, &LuaScriptForm::onButtonClicked, Qt::UniqueConnection);
	}

	connect(ui.treeWidget_script->header(), &QHeaderView::sectionClicked, this, &LuaScriptForm::onScriptTreeWidgetHeaderClicked);
	connect(ui.treeWidget_script, &QTreeWidget::itemDoubleClicked, this, &LuaScriptForm::onScriptTreeWidgetDoubleClicked);

	connect(ui.tableWidget_script, &QTableWidget::itemClicked, this, &LuaScriptForm::onScriptTableWidgetClicked);
	connect(ui.tableWidget_script, &QTableWidget::currentItemChanged, this, &LuaScriptForm::onCurrentTableWidgetItemChanged);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::scriptLabelRowTextChanged, this, &LuaScriptForm::onScriptLabelRowTextChanged, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptContentChanged, this, &LuaScriptForm::onScriptContentChanged, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::loadFileToTable, this, &LuaScriptForm::loadFile, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::reloadScriptList, this, &LuaScriptForm::onReloadScriptList, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptStarted, this, &LuaScriptForm::onScriptStarted, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptStoped, this, &LuaScriptForm::onScriptStoped, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptPaused, this, &LuaScriptForm::onScriptPaused, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptResumed, this, &LuaScriptForm::onScriptResumed, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptBreaked, this, &LuaScriptForm::onScriptResumed, Qt::QueuedConnection);
	emit signalDispatcher.reloadScriptList();

	const QString fileName(qgetenv("JSON_PATH"));
	if (fileName.isEmpty())
		return;

	if (!QFile::exists(fileName))
		return;
	util::Config config(fileName);

	Injector& injector = Injector::getInstance();
	injector.currentScriptFileName = config.read<QString>(objectName(), "LastModifyFile");
	if (!injector.currentScriptFileName.isEmpty() && QFile::exists(injector.currentScriptFileName))
	{
		emit signalDispatcher.loadFileToTable(injector.currentScriptFileName);
	}
}

LuaScriptForm::~LuaScriptForm()
{
}


void LuaScriptForm::onScriptStarted()
{
	Injector& injector = Injector::getInstance();
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

	clua_.reset(new CLua(content));
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
	QPushButton* pPushButton = qobject_cast<QPushButton*>(sender());
	if (!pPushButton)
		return;

	QString name = pPushButton->objectName();
	if (name.isEmpty())
		return;

	//Injector& injector = Injector::getInstance();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
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
void LuaScriptForm::resizeTableWidgetRow(int max)
{
	ui.tableWidget_script->clear();
	//set header
	QStringList header;
	header << tr("command") << tr("params");
	ui.tableWidget_script->setHorizontalHeaderLabels(header);
	int rowCount = ui.tableWidget_script->rowCount();
	if (rowCount < max)
	{
		for (int row = rowCount; row < max; ++row)
		{
			ui.tableWidget_script->insertRow(row);
		}
	}
	else if (rowCount > max)
	{
		for (int row = rowCount - 1; row >= max; --row)
		{
			ui.tableWidget_script->removeRow(row);
		}
	}
}

//設置指定單元格內容
void LuaScriptForm::setTableWidgetItem(int row, int col, const QString& text)
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
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.reloadScriptList();
	//qDebug() << "onScriptTreeWidgetClicked" << logicalIndex;
}

//更新當前行號label
void LuaScriptForm::onScriptLabelRowTextChanged(int row, int max, bool noSelect)
{
	//qDebug() << "onScriptLabelRowTextChanged" << row << max << noSelect;

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

	Injector& injector = Injector::getInstance();
	QString content;
	bool isPrivate = false;
	if (!fileName.contains(util::SCRIPT_LUA_SUFFIX_DEFAULT))
		return;

	if (!util::readFile(fileName, &content, &isPrivate))
		return;

	injector.currentScriptFileName = fileName;


	if (clua_.isNull())
	{
		clua_.reset(new CLua(content));
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.scriptContentChanged(fileName, QVariant::fromValue(content.split("\n")));
}

void LuaScriptForm::onScriptContentChanged(const QString& fileName, const QVariant& vtokens)
{
	QStringList tokens = vtokens.value<QStringList>();

	int rowCount = tokens.size();

	resizeTableWidgetRow(rowCount);

	for (int row = 0; row < rowCount; ++row)
	{
		setTableWidgetItem(row, 0, tokens.at(row).trimmed());
	}

	QString newFileName = fileName;
	newFileName.replace("\\", "/");

	int index = newFileName.indexOf("script/");
	if (index >= 0)
	{
		QString shortPath = fileName.mid(index + 7);
		ui.label_path->setText(shortPath);
		onScriptLabelRowTextChanged(1, rowCount, false);
	}
}

void LuaScriptForm::onCurrentTableWidgetItemChanged(QTableWidgetItem* current, QTableWidgetItem*)
{
	//if (!current)
	//	return;
	//int row = current->row();
	//selectedRow_ = row;

	//Injector& injector = Injector::getInstance();
	//if (injector.IS_SCRIPT_FLAG.load(std::memory_order_acquire))
	//	return;

	//ui.pushButton_script_start->setText(tr("start"));

	//int rowCount = ui.tableWidget_script->rowCount();
	//onScriptLabelRowTextChanged(row + 1, rowCount, true);
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
		Injector& injector = Injector::getInstance();
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
		int count = (filepath.size() - 1);
		for (int i = count; i >= 0; i--) //QStringlist類filepath反向存著初始item的路徑
		{ //將filepath反向輸出，相應的加入’/‘
			if (filepath.at(i).isEmpty())
				continue;
			strpath += filepath.at(i);
			if (i != 0)
				strpath += "/";
		}

		strpath = util::applicationDirPath() + "/script/" + strpath;
		strpath.replace("*", "");

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
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
		for (int i = 0; i < item->childCount(); ++i)
		{
			ui.treeWidget_script->expandItem(item->child(i));
		}

		ui.treeWidget_script->sortItems(0, Qt::AscendingOrder);

		ui.treeWidget_script->setUpdatesEnabled(true);
	} while (false);
}