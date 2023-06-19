#include "stdafx.h"
#include "scriptform.h"

#include "util.h"
#include "script/interpreter.h"

#include "injector.h"

#include "signaldispatcher.h"


ScriptForm::ScriptForm(QWidget* parent)
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
			connect(button, &QPushButton::clicked, this, &ScriptForm::onButtonClicked, Qt::UniqueConnection);
	}

	connect(ui.treeWidget_script->header(), &QHeaderView::sectionClicked, this, &ScriptForm::onScriptTreeWidgetHeaderClicked);
	connect(ui.treeWidget_script, &QTreeWidget::itemDoubleClicked, this, &ScriptForm::onScriptTreeWidgetDoubleClicked);

	connect(ui.tableWidget_script, &QTableWidget::itemClicked, this, &ScriptForm::onScriptTableWidgetClicked);
	connect(ui.tableWidget_script, &QTableWidget::currentItemChanged, this, &ScriptForm::onCurrentTableWidgetItemChanged);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::scriptLabelRowTextChanged, this, &ScriptForm::onScriptLabelRowTextChanged, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptPaused, this, &ScriptForm::onScriptPaused, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptContentChanged, this, &ScriptForm::onScriptContentChanged, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::loadFileToTable, this, &ScriptForm::loadFile, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::reloadScriptList, this, &ScriptForm::onReloadScriptList, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptStarted, this, &ScriptForm::onStartScript, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptStoped, ui.pushButton_script_stop, &QPushButton::click, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptPaused2, ui.pushButton_script_pause, &QPushButton::click, Qt::QueuedConnection);
	emit signalDispatcher.reloadScriptList();
}

ScriptForm::~ScriptForm()
{
}

void ScriptForm::onScriptPaused()
{
	if (!interpreter_->isPaused())
	{
		ui.pushButton_script_pause->setText(tr("resume"));
	}
	else
	{
		ui.pushButton_script_pause->setText(tr("pause"));
	}
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
	QPushButton* pPushButton = qobject_cast<QPushButton*>(sender());
	if (!pPushButton)
		return;

	QString name = pPushButton->objectName();
	if (name.isEmpty())
		return;

	Injector& injector = Injector::getInstance();

	if (name == "pushButton_script_start")
	{
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.scriptStarted();
	}
	else if (name == "pushButton_script_pause")
	{
		if (!interpreter_.isNull())
		{
			onScriptPaused();
			if (!interpreter_->isPaused())
			{
				interpreter_->pause();
			}
			else
			{
				interpreter_->resume();
			}
		}

	}
	else if (name == "pushButton_script_stop")
	{
		if (!interpreter_.isNull())
		{
			interpreter_->requestInterruption();
			if (interpreter_->isPaused())
				interpreter_->resume();
		}
	}
}

//重設表格最大行數
void ScriptForm::resizeTableWidgetRow(int max)
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
void ScriptForm::setTableWidgetItem(int row, int col, const QString& text)
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
void ScriptForm::onScriptTreeWidgetHeaderClicked(int logicalIndex)
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.reloadScriptList();
	//qDebug() << "onScriptTreeWidgetClicked" << logicalIndex;
}

//更新當前行號label
void ScriptForm::onScriptLabelRowTextChanged(int row, int max, bool noSelect)
{
	//qDebug() << "onScriptLabelRowTextChanged" << row << max << noSelect;
	ui.label_row->setUpdatesEnabled(false);
	ui.label_row->setText(QString("%1/%2").arg(row).arg(max));
	if (!noSelect)
	{
		ui.tableWidget_script->selectRow(row - 1);
	}

	ui.label_row->setUpdatesEnabled(true);

}

//加載並預覽腳本
void ScriptForm::loadFile(const QString& fileName)
{
	if (fileName.isEmpty())
		return;

	if (interpreter_.isNull())
	{
		interpreter_.reset(new Interpreter());
	}
	currentFileName_ = fileName;
	interpreter_->preview(fileName);
}

void ScriptForm::onStartScript()
{
	if (currentFileName_.isEmpty())
		return;

	if (!interpreter_.isNull())
	{
		interpreter_->requestInterruption();
	}

	if (interpreter_->isRunning())
	{
		return;
	}

	Injector& injector = Injector::getInstance();
	if (!injector.scriptLogModel.isNull())
		injector.scriptLogModel->clear();

	interpreter_.reset(new Interpreter());

	connect(interpreter_.data(), &Interpreter::finished, this, &ScriptForm::onScriptFinished);

	interpreter_->doFileWithThread(selectedRow_, currentFileName_);

	ui.pushButton_script_start->setEnabled(false);
	ui.pushButton_script_pause->setEnabled(true);
	ui.pushButton_script_stop->setEnabled(true);
}

void ScriptForm::onScriptContentChanged(const QString& fileName, const QVariant& vtokens)
{
	QHash<int, TokenMap> tokens = vtokens.value<QHash<int, TokenMap>>();

	int rowCount = tokens.size();

	resizeTableWidgetRow(rowCount);

	for (int row = 0; row < rowCount; ++row)
	{
		TokenMap lineTokens = tokens.value(row);

		QStringList params;
		int size = lineTokens.size();
		for (int i = 1; i < size; ++i)
		{
			params.append(lineTokens.value(i).raw);
		}

		setTableWidgetItem(row, 0, lineTokens.value(0).raw);
		setTableWidgetItem(row, 1, params.join(", "));
	}

	int index = fileName.indexOf("script/");
	QString shortPath = fileName.mid(index + 7);
	ui.label_path->setText(shortPath);
	onScriptLabelRowTextChanged(1, rowCount, false);
}

void ScriptForm::onCurrentTableWidgetItemChanged(QTableWidgetItem* current, QTableWidgetItem* previous)
{
	if (!current)
		return;
	int row = current->row();
	selectedRow_ = row;

	Injector& injector = Injector::getInstance();
	if (injector.IS_SCRIPT_FLAG)
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

void ScriptForm::onScriptTableWidgetClicked(QTableWidgetItem* item)
{
	if (!item)
		return;
	int row = item->row();
}

//樹型框雙擊事件
void ScriptForm::onScriptTreeWidgetDoubleClicked(QTreeWidgetItem* item, int column)
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
		if (injector.IS_SCRIPT_FLAG)
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

		strpath = QApplication::applicationDirPath() + "/script/" + strpath;
		strpath.replace("*", "");

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.loadFileToTable(strpath);
		//ui.widget->clear();
		//this->setWindowTitle(QString("[%1] %2").arg(index_).arg(currentFileName_));
		//ui.widget->convertEols(QsciScintilla::EolWindows);
		//ui.widget->setUtf8(true);
		//ui.widget->setModified(false);
		//ui.widget->setText(content);
	} while (false);

	IS_LOADING = false;
}

//重新加載腳本列表
void ScriptForm::onReloadScriptList()
{
	if (IS_LOADING) return;

	TreeWidgetItem* item = nullptr;
	QStringList newScriptList = {};
	do
	{
		item = q_check_ptr(new TreeWidgetItem);
		if (!item) break;

		util::loadAllFileLists(item, QApplication::applicationDirPath() + "/script/", &newScriptList);
		//compare with oldScriptList
		if (scriptList_ == newScriptList)
		{
			delete item;
			item = nullptr;
			break;
		}

		scriptList_ = newScriptList;
		ui.treeWidget_script->clear();
		ui.treeWidget_script->addTopLevelItem(item);
		//展開全部第一層
		ui.treeWidget_script->topLevelItem(0)->setExpanded(true);
		for (int i = 0; i < item->childCount(); i++)
		{
			ui.treeWidget_script->expandItem(item->child(i));
		}

		ui.treeWidget_script->sortItems(0, Qt::AscendingOrder);
	} while (false);
}