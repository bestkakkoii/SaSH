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
#include "scriptform.h"
#include "util.h"
#include "script/interpreter.h"
#include "injector.h"
#include "signaldispatcher.h"

ScriptForm::ScriptForm(long long index, QWidget* parent)
	: QWidget(parent)
	, Indexer(index)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_StyledBackground);

	QList<PushButton*> buttonList = util::findWidgets<PushButton>(this);
	for (auto& button : buttonList)
	{
		if (button)
			connect(button, &PushButton::clicked, this, &ScriptForm::onButtonClicked, Qt::QueuedConnection);
	}

	ui.tableWidget_script->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
	ui.tableWidget_script->horizontalHeader()->setStretchLastSection(true);
	ui.tableWidget_script->setCornerText(tr("row"));

	ui.label_row->setAutoResize(true);

	connect(ui.treeWidget_script->header(), &QHeaderView::sectionClicked, this, &ScriptForm::onScriptTreeWidgetHeaderClicked);
	connect(ui.treeWidget_script, &QTreeWidget::itemDoubleClicked, this, &ScriptForm::onScriptTreeWidgetDoubleClicked);

	connect(ui.tableWidget_script, &QTableWidget::itemClicked, this, &ScriptForm::onScriptTableWidgetClicked);
	connect(ui.tableWidget_script, &QTableWidget::currentItemChanged, this, &ScriptForm::onCurrentTableWidgetItemChanged);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	connect(&signalDispatcher, &SignalDispatcher::scriptLabelRowTextChanged, this, &ScriptForm::onScriptLabelRowTextChanged, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptContentChanged, this, &ScriptForm::onScriptContentChanged, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::loadFileToTable, this, &ScriptForm::loadFile, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::reloadScriptList, this, &ScriptForm::onReloadScriptList, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptStarted, this, &ScriptForm::onScriptStarted, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptStoped, this, &ScriptForm::onScriptStoped, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptPaused, this, &ScriptForm::onScriptPaused, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptResumed, this, &ScriptForm::onScriptResumed, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptBreaked, this, &ScriptForm::onScriptResumed, Qt::QueuedConnection);

	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &ScriptForm::onApplyHashSettingsToUI, Qt::QueuedConnection);
	emit signalDispatcher.reloadScriptList();

	connect(ui.spinBox_speed, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ScriptForm::onSpeedChanged);

	util::Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));

	Injector& injector = Injector::getInstance(index);
	QString currentScriptFileName = config.read<QString>("Script", "LastModifyFile");

	if (currentScriptFileName.isEmpty() || !QFile::exists(currentScriptFileName))
	{
		QString defaultScriptPath = util::applicationDirPath() + "/script/default.txt";
		util::ScopedFile fileDefault(defaultScriptPath);
		if (!fileDefault.exists())
		{
			if (!fileDefault.openWriteNew())
				return;
			fileDefault.close();
		}

		currentScriptFileName = defaultScriptPath;
	}

	injector.currentScriptFileName = currentScriptFileName;
	emit signalDispatcher.loadFileToTable(currentScriptFileName);
}

ScriptForm::~ScriptForm()
{
}

void ScriptForm::onScriptStarted()
{
	long long currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.IS_SCRIPT_FLAG.get())
		return;

	if (interpreter_ != nullptr)
	{
		if (interpreter_->isRunning())
			return;
	}

	interpreter_.reset(q_check_ptr(new Interpreter(currentIndex)));
	sash_assume(interpreter_ != nullptr);
	if (nullptr == interpreter_)
		return;

	injector.scriptLogModel.clear();

	connect(interpreter_.get(), &Interpreter::finished, this, &ScriptForm::onScriptFinished, Qt::QueuedConnection);

	interpreter_->doFileWithThread(selectedRow_, injector.currentScriptFileName);

	ui.pushButton_script_start->setEnabled(false);
	ui.pushButton_script_pause->setEnabled(true);
	ui.pushButton_script_stop->setEnabled(true);
}

void ScriptForm::onScriptPaused()
{
	Injector& injector = Injector::getInstance(getIndex());
	if (!injector.IS_SCRIPT_FLAG.get())
		return;

	if (interpreter_ != nullptr)
	{
		if (!injector.isPaused())
		{
			ui.pushButton_script_pause->setText(tr("resume"));
			injector.paused();
		}
	}
}

void ScriptForm::onScriptResumed()
{
	Injector& injector = Injector::getInstance(getIndex());
	if (!injector.IS_SCRIPT_FLAG.get())
		return;

	if (interpreter_ != nullptr)
	{

		if (injector.isPaused())
		{
			ui.pushButton_script_pause->setText(tr("pause"));
			injector.resumed();
		}
	}
}

void ScriptForm::onScriptStoped()
{
	Injector& injector = Injector::getInstance(getIndex());
	if (!injector.IS_SCRIPT_FLAG.get())
		return;

	injector.stopScript();
	if (injector.isPaused())
		injector.resumed();
}

//腳本結束信號槽
void ScriptForm::onScriptFinished()
{
	selectedRow_ = 0;
	ui.pushButton_script_start->setText(tr("start"));
	ui.pushButton_script_pause->setText(tr("pause"));
	ui.pushButton_script_start->setEnabled(true);
	ui.pushButton_script_pause->setEnabled(false);
	ui.pushButton_script_stop->setEnabled(false);
}

void ScriptForm::onButtonClicked()
{
	PushButton* pPushButton = qobject_cast<PushButton*>(sender());
	if (!pPushButton)
		return;

	QString name = pPushButton->objectName();
	if (name.isEmpty())
		return;

	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	if (name == "pushButton_script_start")
	{
		Injector& injector = Injector::getInstance(currentIndex);
		if (!injector.IS_SCRIPT_FLAG.get() && QFile::exists(injector.currentScriptFileName))
			emit signalDispatcher.scriptStarted();
	}
	else if (name == "pushButton_script_pause")
	{
		Injector& injector = Injector::getInstance(currentIndex);
		if (!injector.isPaused())
			emit signalDispatcher.scriptPaused();
		else
			emit signalDispatcher.scriptResumed();
	}
	else if (name == "pushButton_script_stop")
	{
		emit signalDispatcher.scriptStoped();
	}
}

//重設表格最大行數
void ScriptForm::resizeTableWidgetRow(long long max)
{
	ui.tableWidget_script->clear();
	//set header
	QStringList header;
	header << tr("command") << tr("params");
	ui.tableWidget_script->setHorizontalHeaderLabels(header);
	long long rowCount = ui.tableWidget_script->rowCount();
	if (rowCount < max)
	{
		for (long long row = rowCount; row < max; ++row)
		{
			ui.tableWidget_script->insertRow(row);
		}
	}
	else if (rowCount > max)
	{
		for (long long row = rowCount - 1; row >= max; --row)
		{
			ui.tableWidget_script->removeRow(row);
		}
	}
}

//樹型框header點擊信號槽
void ScriptForm::onScriptTreeWidgetHeaderClicked(int)
{
	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.reloadScriptList();
	//qDebug() << "onScriptTreeWidgetClicked" << logicalIndex;
}

//更新當前行號label
void ScriptForm::onScriptLabelRowTextChanged(long long row, long long max, bool noSelect)
{
	ui.label_row->setText(QString("%1/%2").arg(row).arg(max));
	if (!noSelect)
	{
		ui.tableWidget_script->selectRow(row - 1);
	}
}

//加載並預覽腳本
void ScriptForm::loadFile(const QString& fileName, bool start)
{
	if (fileName.isEmpty())
		return;

	long long currentIndex = getIndex();
	if (interpreter_ == nullptr)
	{
		interpreter_.reset(q_check_ptr(new Interpreter(currentIndex)));
		sash_assume(interpreter_ != nullptr);
		if (nullptr == interpreter_)
			return;
	}

	interpreter_->preview(fileName);
	if (start)
	{
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.scriptStarted();
	}
}

void ScriptForm::onScriptContentChanged(const QString& fileName, const QVariant& vtokens)
{
	QHash<long long, TokenMap> tokens = vtokens.value<QHash<long long, TokenMap>>();

	long long rowCount = tokens.size();

	resizeTableWidgetRow(rowCount);
	long long i = 0;
	for (long long row = 0; row < rowCount; ++row)
	{
		TokenMap lineTokens = tokens.value(row);

		QStringList params;
		long long size = lineTokens.size();

		if (lineTokens.value(0).type == TK_COMMENT)
		{
			ui.tableWidget_script->setText(row, 0, "[comment]");
			ui.tableWidget_script->setText(row, 1, lineTokens.value(1).raw.simplified());
			continue;
		}

		if (lineTokens.contains(100))
		{
			params.append(lineTokens.value(100).raw.simplified());
		}
		else
		{
			for (i = 1; i < size; ++i)
			{
				QString str = lineTokens.value(i).raw.simplified();
				RESERVE reserve = lineTokens.value(i).type;
				if (reserve != TK_CSTRING && !str.isEmpty())
					params.append(str);
				else
					params.append(QString("\'%1\'").arg(str));
			}
		}


		RESERVE reserve = lineTokens.value(0).type;
		QString cmd = lineTokens.value(0).raw.simplified();
		QString argStr = params.join(", ");
		if (TK_LOCAL == reserve || TK_LOCALTABLE == reserve)
		{
			ui.tableWidget_script->setText(row, 0, "[local]");
			if (!argStr.isEmpty())
				ui.tableWidget_script->setText(row, 1, QString("%1 = %2").arg(cmd, argStr));
			else
				ui.tableWidget_script->setText(row, 1, QString("%1 = \'\'").arg(cmd));
			continue;
		}
		else if (TK_MULTIVAR == reserve || TK_TABLE == reserve || TK_GLOBAL == reserve)
		{
			ui.tableWidget_script->setText(row, 0, "[global]");
			if (!argStr.isEmpty())
				ui.tableWidget_script->setText(row, 1, QString("%1 = %2").arg(cmd, argStr));
			else
				ui.tableWidget_script->setText(row, 1, QString("%1 = \'\'").arg(cmd));
			continue;
		}
		else if (cmd.size() <= 12)
			ui.tableWidget_script->setText(row, 0, cmd);
		else
		{
			ui.tableWidget_script->setText(row, 0, "[long command]");
			ui.tableWidget_script->setText(row, 1, QString("%1 %2").arg(cmd, argStr));
			continue;
		}

		ui.tableWidget_script->setText(row, 1, argStr);
	}

	QString newFileName = fileName;
	newFileName.replace("\\", "/");

	long long index = newFileName.indexOf("script/");
	if (index >= 0)
	{
		QString shortPath = fileName.mid(index + 7);
		ui.label_path->setText(shortPath);
		onScriptLabelRowTextChanged(1, rowCount, false);
	}
}

void ScriptForm::onCurrentTableWidgetItemChanged(QTableWidgetItem* current, QTableWidgetItem*)
{
	if (!current)
		return;
	long long row = current->row();
	selectedRow_ = row;
	long long currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.IS_SCRIPT_FLAG.get())
		return;

	if (row == 0)
	{
		ui.pushButton_script_start->setText(tr("start"));
	}
	else
	{
		ui.pushButton_script_start->setText(tr("mid-start"));
	}

	int rowCount = ui.tableWidget_script->rowCount();
	onScriptLabelRowTextChanged(row + 1, rowCount, true);
}

void ScriptForm::onScriptTableWidgetClicked(QTableWidgetItem*)
{
}

//樹型框雙擊事件
void ScriptForm::onScriptTreeWidgetDoubleClicked(QTreeWidgetItem* item, int column)
{
	if (!item)
		return;

	std::ignore = column;
	if (item->text(0).isEmpty())
		return;

	do
	{
		long long currentIndex = getIndex();
		Injector& injector = Injector::getInstance(currentIndex);
		if (injector.IS_SCRIPT_FLAG.get())
			break;

		/*得到文件路徑*/
		QStringList filepath;
		TreeWidgetItem* itemfile = reinterpret_cast<TreeWidgetItem*>(item); //獲取被點擊的item
		for (;;)
		{
			if (nullptr == itemfile)
				break;

			filepath << itemfile->text(0); //獲取itemfile名稱
			itemfile = reinterpret_cast<TreeWidgetItem*>(itemfile->parent()); //將itemfile指向父item
		}
		QString strpath;
		long long count = static_cast<long long>(filepath.size()) - 1;
		for (long long i = count; i >= 0; i--) //QStringlist類filepath反向存著初始item的路徑
		{ //將filepath反向輸出，相應的加入’/‘
			if (filepath.value(i).isEmpty())
				continue;
			strpath += filepath.value(i);
			if (i != 0)
				strpath += "/";
		}

		strpath = util::applicationDirPath() + "/script/" + strpath;
		strpath.replace("*", "");

		QFileInfo fileinfo(strpath);
		if (!fileinfo.isFile())
			break;

		if (!injector.IS_SCRIPT_FLAG.get())
			injector.currentScriptFileName = strpath;

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
		emit signalDispatcher.loadFileToTable(strpath);
	} while (false);
}

//重新加載腳本列表
void ScriptForm::onReloadScriptList()
{
	QStringList newScriptList = {};
	TreeWidgetItem* item = q_check_ptr(new TreeWidgetItem());
	sash_assume(item != nullptr);
	if (nullptr == item)
		return;

	util::loadAllFileLists(item, util::applicationDirPath() + "/script/", &newScriptList);

	scriptList_ = newScriptList;
	ui.treeWidget_script->clear();
	ui.treeWidget_script->addTopLevelItem(item);

	item->setExpanded(true);

	ui.treeWidget_script->sortItems(0, Qt::AscendingOrder);
}

void ScriptForm::onSpeedChanged(int value)
{
	long long currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	injector.setValueHash(util::kScriptSpeedValue, value);

	emit SignalDispatcher::getInstance(currentIndex).scriptSpeedChanged(value);
}

void ScriptForm::onApplyHashSettingsToUI()
{
	long long currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	QHash<util::UserSetting, bool> enableHash = injector.getEnablesHash();
	QHash<util::UserSetting, long long> valueHash = injector.getValuesHash();
	QHash<util::UserSetting, QString> stringHash = injector.getStringsHash();

	ui.spinBox_speed->setValue(valueHash.value(util::kScriptSpeedValue));
}