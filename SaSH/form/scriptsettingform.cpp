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
#include "scriptsettingform.h"
#include "model/customtitlebar.h"

#include "injector.h"
#include "signaldispatcher.h"

//#include "crypto.h"
#include <QSpinBox>

extern util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>> break_markers;//interpreter.cpp//用於標記自訂義中斷點(紅點)
extern util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>> forward_markers;//interpreter.cpp//用於標示當前執行中斷處(黃箭頭)
extern util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>> error_markers;//interpreter.cpp//用於標示錯誤發生行(紅線)
extern util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>> step_markers;//interpreter.cpp//隱式標記中斷點用於單步執行(無)

ScriptSettingForm::ScriptSettingForm(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowFlags(Qt::FramelessWindowHint);

	CustomTitleBar* titleBar = new CustomTitleBar(CustomTitleBar::kAllButton, this);
	setMenuWidget(titleBar);

	//Qt::WindowFlags windowflag = this->windowFlags();
	//windowflag |= Qt::WindowType::Tool;
	//setWindowFlag(Qt::WindowType::Tool);

	qRegisterMetaType<QVector<int>>();

	installEventFilter(this);

	//reset font size
	QFont font = ui.listView_log->font();
	font.setPointSize(12);
	ui.listView_log->setFont(font);



	takeCentralWidget();
	setDockNestingEnabled(true);
	addDockWidget(Qt::LeftDockWidgetArea, ui.dockWidget_functionList);
	addDockWidget(Qt::RightDockWidgetArea, ui.dockWidget_scriptList);
	addDockWidget(Qt::BottomDockWidgetArea, ui.dockWidget_scriptFun);

	splitDockWidget(ui.dockWidget_functionList, ui.dockWidget_main, Qt::Horizontal);
	splitDockWidget(ui.dockWidget_main, ui.dockWidget_scriptList, Qt::Horizontal);
	splitDockWidget(ui.dockWidget_functionList, ui.dockWidget_debuger, Qt::Vertical);
	splitDockWidget(ui.dockWidget_main, ui.dockWidget_scriptFun, Qt::Vertical);
	ui.dockWidget_functionList->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	ui.dockWidget_main->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	ui.dockWidget_scriptList->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	ui.dockWidget_debuger->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	ui.dockWidget_scriptFun->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

	tabifyDockWidget(ui.dockWidget_scriptFun, ui.dockWidget_des);
	tabifyDockWidget(ui.dockWidget_scriptFun, ui.dockWidget_mark);
	//ui.dockWidget_debuger->hide();

	ui.treeWidget_scriptList->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui.treeWidget_scriptList->resizeColumnToContents(1);
	ui.treeWidget_scriptList->header()->setSectionsClickable(true);
	connect(ui.treeWidget_scriptList->header(), &QHeaderView::sectionClicked, this, &ScriptSettingForm::onScriptTreeWidgetHeaderClicked);
	connect(ui.treeWidget_scriptList, &QTreeWidget::itemDoubleClicked, this, &ScriptSettingForm::onScriptTreeWidgetDoubleClicked);
	connect(ui.treeWidget_scriptList, &QTreeWidget::itemChanged, this, &ScriptSettingForm::onScriptTreeWidgetItemChanged);
	//排序
	ui.treeWidget_functionList->sortItems(0, Qt::AscendingOrder);


	QGridLayout* gridLayout = new QGridLayout;
	gridLayout->addWidget(ui.listView_log);
	gridLayout->setMargin(0);

	ui.openGLWidget_2->setLayout(gridLayout);

	QGridLayout* gridLayoutDebug = new QGridLayout;
	gridLayoutDebug->addWidget(ui.treeWidget_debuger_custom);
	gridLayoutDebug->setMargin(0);

	ui.openGLWidget_3->setLayout(gridLayoutDebug);


	ui.menuBar->setMinimumWidth(200);

	//載入固定狀態
	staticLabel_.setParent(this);
	staticLabel_.setFrameStyle(QFrame::NoFrame);
	staticLabel_.setText(QString(tr("row:%1 | size:%2 | index:%3 | %4").arg(1).arg(0).arg(1).arg("CRLF")));
	staticLabel_.setOpenExternalLinks(true);
	staticLabel_.setStyleSheet("color: rgb(255, 255, 255); background-color: rgb(134, 27, 45); border:none");
	ui.statusBar->addPermanentWidget(&staticLabel_);
	ui.statusBar->setStyleSheet("color: rgb(255, 255, 255); background-color: rgb(134, 27, 45); border:none");


	//ui.listView_log->setModel(thread->getScriptLogModel());

	ui.listView_log->setWordWrap(true);
	ui.listView_log->setUniformItemSizes(true);
	ui.listView_log->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	ui.listView_log->setTextElideMode(Qt::ElideNone);
	ui.listView_log->setResizeMode(QListView::Adjust);
	ui.listView_log->installEventFilter(this);


	ui.mainToolBar->show();


	//ui.textBrowser->setDocument(document_.);

	connect(ui.actionSave, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionSaveAs, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionDirectory, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionStep, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionMark, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionAutoCleanLog, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionAutoFollow, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionNew, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionStart, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionPause, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionStop, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionContinue, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionLogback, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionSaveEncode, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionSaveDecode, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionDebug, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);

	connect(this, &ScriptSettingForm::editorCursorPositionChanged, this, &ScriptSettingForm::onEditorCursorPositionChanged, Qt::QueuedConnection);
	connect(this, &ScriptSettingForm::breakMarkInfoImport, this, &ScriptSettingForm::onBreakMarkInfoImport, Qt::QueuedConnection);


	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &ScriptSettingForm::onApplyHashSettingsToUI, Qt::UniqueConnection);

	connect(&signalDispatcher, &SignalDispatcher::scriptStarted, this, &ScriptSettingForm::onScriptStartMode, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptPaused, this, &ScriptSettingForm::onScriptPauseMode, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptResumed, this, &ScriptSettingForm::onScriptStartMode, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptBreaked, this, &ScriptSettingForm::onScriptBreakMode, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptFinished, this, &ScriptSettingForm::onScriptStopMode, Qt::QueuedConnection);

	connect(&signalDispatcher, &SignalDispatcher::addForwardMarker, this, &ScriptSettingForm::onAddForwardMarker, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::addErrorMarker, this, &ScriptSettingForm::onAddErrorMarker, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::addBreakMarker, this, &ScriptSettingForm::onAddBreakMarker, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::addStepMarker, this, &ScriptSettingForm::onAddStepMarker, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::loadFileToTable, this, &ScriptSettingForm::loadFile, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::reloadScriptList, this, &ScriptSettingForm::onReloadScriptList, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptLabelRowTextChanged, this, &ScriptSettingForm::onScriptLabelRowTextChanged, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::varInfoImported, this, &ScriptSettingForm::onVarInfoImport, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::callStackInfoChanged, this, &ScriptSettingForm::onCallStackInfoChanged, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::jumpStackInfoChanged, this, &ScriptSettingForm::onJumpStackInfoChanged, Qt::QueuedConnection);

	Injector& injector = Injector::getInstance();
	if (!injector.scriptLogModel.isNull())
		ui.listView_log->setModel(injector.scriptLogModel.get());

	createScriptListContextMenu();

	emit signalDispatcher.reloadScriptList();

	util::FormSettingManager formSettingManager(this);
	formSettingManager.loadSettings();

	onScriptStopMode();
	onApplyHashSettingsToUI();
	//ui.webEngineView->setUrl(QUrl("https://gitee.com/Bestkakkoii/sash/wikis/pages"));

	if (!injector.currentScriptFileName.isEmpty() && QFile::exists(injector.currentScriptFileName))
	{
		emit signalDispatcher.loadFileToTable(injector.currentScriptFileName);
	}

	const QString fileName(qgetenv("JSON_PATH"));
	if (fileName.isEmpty())
		return;

	if (!QFile::exists(fileName))
		return;
	util::Config config(fileName);

	QString ObjectName = ui.widget->objectName();
	int fontSize = config.read<int>(objectName(), ObjectName, "FontSize");
	if (fontSize > 0)
	{
		font = ui.widget->getOldFont();
		font.setPointSize(fontSize);
		ui.widget->setNewFont(font);
		ui.widget->zoomTo(3);
	}

	ObjectName = ui.listView_log->objectName();
	fontSize = config.read<int>(objectName(), ObjectName, "FontSize");
	if (fontSize > 0)
	{
		QFont f = ui.listView_log->font();
		f.setPointSize(fontSize);
		ui.listView_log->setFont(f);
	}

	ObjectName = ui.textBrowser->objectName();
	fontSize = config.read<int>(objectName(), ObjectName, "FontSize");
	if (fontSize > 0)
	{
		font = ui.textBrowser->font();
		font.setPointSize(fontSize);
		ui.textBrowser->setFont(font);
	}
}

void ScriptSettingForm::createSpeedSpinBox()
{
	pSpeedSpinBox = new QSpinBox(this);
	pSpeedSpinBox->setRange(0, 10000);
	Injector& injector = Injector::getInstance();
	int value = injector.getValueHash(util::kScriptSpeedValue);
	pSpeedSpinBox->setValue(value);
	pSpeedSpinBox->setStyleSheet(R"(
		QSpinBox {
			padding-top: 2px;
			padding-bottom: 2px;
			padding-left: 4px;
			padding-right: 15px;
			border:1px solid rgb(66,66,66);
 			color:rgb(250,250,250);
  			background: rgb(56,56,56);
			selection-color: rgb(208,208,208);
			selection-background-color: rgb(80, 80, 83);
			font-family: "Microsoft Yahei";
			font-size: 10pt;
		}

		QSpinBox:hover {
		  color:rgb(250,250,250);
		  background: rgb(31,31,31);
		   border:1px solid rgb(153,153,153);
		}
	)");

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::scriptSpeedChanged, this, &ScriptSettingForm::onSpeedChanged);

	connect(pSpeedSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ScriptSettingForm::onSpeedChanged);

	ui.mainToolBar->addWidget(pSpeedSpinBox);
}

ScriptSettingForm::~ScriptSettingForm()
{
	qDebug() << "~ScriptSettingForm";
}

void ScriptSettingForm::showEvent(QShowEvent* e)
{
	setAttribute(Qt::WA_Mapped);
	QMainWindow::showEvent(e);
}

void ScriptSettingForm::closeEvent(QCloseEvent* e)
{
	do
	{
		util::FormSettingManager formSettingManager(this);
		formSettingManager.saveSettings();

		const QString fileName(qgetenv("JSON_PATH"));
		if (fileName.isEmpty())
			break;

		if (!QFile::exists(fileName))
			break;

		util::Config config(fileName);
		Injector& injector = Injector::getInstance();
		config.write(objectName(), "LastModifyFile", injector.currentScriptFileName);


		QString ObjectName = ui.widget->objectName();
		QFont font = ui.widget->getOldFont();
		config.write(objectName(), ObjectName, "FontSize", font.pointSize());

		ObjectName = ui.listView_log->objectName();
		font = ui.listView_log->font();
		config.write(objectName(), ObjectName, "FontSize", font.pointSize());

		ObjectName = ui.textBrowser->objectName();
		font = ui.textBrowser->font();
		config.write(objectName(), ObjectName, "FontSize", font.pointSize());
	} while (false);

	QMainWindow::closeEvent(e);
}

bool ScriptSettingForm::eventFilter(QObject* obj, QEvent* e)
{
	if (obj == this)
	{
		////窗口状态被改变的事件.

		//if (e->type() == QEvent::WindowStateChange)
		//{
		//	if (this->windowState() == Qt::WindowMinimized)
		//	{
		//		this->hide();

		//		return true;
		//	}
		//}
	}
	else if (obj == ui.listView_log)
	{
		if (e->type() == QEvent::KeyPress)
		{
			QKeyEvent* keyEvent = reinterpret_cast<QKeyEvent*>(e);
			if (keyEvent->key() == Qt::Key_Delete)
			{
				Injector& injector = Injector::getInstance();
				if (!injector.scriptLogModel.isNull())
					injector.scriptLogModel->clear();
				return true;
			}
		}
	}
	return QObject::eventFilter(obj, e);
}

bool ScriptSettingForm::nativeEvent(const QByteArray&, void* message, long* result)
{
	MSG* msg = (MSG*)message;
	switch (msg->message)
	{
	case WM_NCHITTEST:
		if (isMaximized())
			return false;

		int xPos = GET_X_LPARAM(msg->lParam) - this->frameGeometry().x();
		int yPos = GET_Y_LPARAM(msg->lParam) - this->frameGeometry().y();
		if (xPos < boundaryWidth_ && yPos < boundaryWidth_)                    //左上角
			*result = HTTOPLEFT;
		else if (xPos >= width() - boundaryWidth_ && yPos < boundaryWidth_)          //右上角
			*result = HTTOPRIGHT;
		else if (xPos < boundaryWidth_ && yPos >= height() - boundaryWidth_)         //左下角
			*result = HTBOTTOMLEFT;
		else if (xPos >= width() - boundaryWidth_ && yPos >= height() - boundaryWidth_)//右下角
			*result = HTBOTTOMRIGHT;
		else if (xPos < boundaryWidth_)                                     //左边
			*result = HTLEFT;
		else if (xPos >= width() - boundaryWidth_)                              //右边
			*result = HTRIGHT;
		else if (yPos < boundaryWidth_)                                       //上边
			*result = HTTOP;
		else if (yPos >= height() - boundaryWidth_)                             //下边
			*result = HTBOTTOM;
		else              //其他部分不做处理，返回false，留给其他事件处理器处理
			return false;
		return true;
	}
	return false;         //此处返回false，留给其他事件处理器处理
}

//////////////////////

void ScriptSettingForm::replaceCommas(QString& input)
{
	static const QRegularExpression regexComma(R"(\s*(,)\s*(?=(?:[^'"]*(['"])(?:[^'"]*\\.)*[^'"]*\2)*[^'"]*$))");
	input.replace(regexComma, ", ");
}

QString ScriptSettingForm::formatCode(const QString& content)
{
	const QStringList contents = content.split("\n");
	QStringList newContents;
	int indentLevel = 0;
	QString indentedLine;
	QString tmpLine;

	for (const QString& line : contents)
	{
		QString trimmedLine = line.trimmed();
		int endIndex = trimmedLine.indexOf("end");
		if (-1 == endIndex)
		{
			endIndex = trimmedLine.indexOf("until");
		}

		tmpLine.clear();
		if (endIndex != -1)
		{
			int commandIndex = trimmedLine.simplified().indexOf("//");
			if (commandIndex == -1)
			{
				commandIndex = trimmedLine.simplified().indexOf("/*");
			}

			if (commandIndex != -1 && commandIndex < endIndex)
			{
				tmpLine = trimmedLine.left(commandIndex).simplified();
			}
			else
			{
				tmpLine = trimmedLine.simplified();
			}
		}

		if (trimmedLine.startsWith("end") && !tmpLine.isEmpty() && tmpLine == "end")
		{
			indentLevel--;
		}
		else if (trimmedLine.startsWith("until") && !tmpLine.isEmpty())
		{
			indentLevel--;
		}

		indentedLine = QString("    ").repeated(indentLevel) + trimmedLine;
		newContents.append(indentedLine);

		if (trimmedLine.startsWith("function ") || trimmedLine.startsWith("local function "))
		{
			indentLevel++;
		}
		else if (trimmedLine.startsWith("for ") || trimmedLine.startsWith("for (") || trimmedLine.startsWith("for("))
		{
			indentLevel++;
		}
		else if (trimmedLine.startsWith("while "))
		{
			indentLevel++;
		}
		else if (trimmedLine.startsWith("repeat "))
		{
			indentLevel++;
		}
	}

	for (QString& line : newContents)
	{
		if (!line.isEmpty())
			replaceCommas(line);
	}

	return newContents.join("\n");
}

void ScriptSettingForm::fileSave(const QString& d, DWORD flag)
{
	Injector& injector = Injector::getInstance();
	if (injector.IS_SCRIPT_FLAG.load(std::memory_order_acquire))
		return;

	const QString directoryName(util::applicationDirPath() + "/script");
	const QDir dir(directoryName);
	if (!dir.exists())
		dir.mkdir(directoryName);

	const QString fileName(injector.currentScriptFileName);//當前路徑

	if (fileName.isEmpty())
		return;

	//backup
	QString applicationDirPath = util::applicationDirPath();
	QDir bakDir(applicationDirPath + "/script/bak");
	if (!bakDir.exists())
		bakDir.mkdir(applicationDirPath + "/script/bak");
	QFileInfo fi(fileName);
	QString backupName(QString("%1/script/bak/%2.bak").arg(applicationDirPath).arg(fi.fileName()));
	QFile::remove(backupName);
	QFile::copy(fileName, backupName);

	util::ScopedFile file(fileName, static_cast<QIODevice::OpenModeFlag>(flag));
	if (!file.isOpen())
		return;

	QString content(d);
	content.replace("\r\n", "\n");

	QTextStream ts(&file);
	ts.setCodec(util::DEFAULT_CODEPAGE);
	ts.setGenerateByteOrderMark(true);
	ts.setLocale(QLocale::Chinese);

	ts << formatCode(content);
	ts.flush();
	file.flush();

	ui.statusBar->showMessage(QString(tr("Script %1 saved")).arg(fileName), 10000);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.addForwardMarker(-1, false);
	emit signalDispatcher.addErrorMarker(-1, false);
	emit signalDispatcher.addStepMarker(-1, false);
	forward_markers.clear();
	error_markers.clear();
	step_markers.clear();
	reshowBreakMarker();

	emit ui.comboBox_labels->clicked();
	emit ui.comboBox_functions->clicked();
	emit ui.widget->modificationChanged(true);
}

void ScriptSettingForm::loadFile(const QString& fileName)
{
	util::ScopedFile f(fileName, QIODevice::ReadOnly | QIODevice::Text);
	if (!f.isOpen())
		return;

	bool isReadOnly = ui.widget->isReadOnly();
	if (isReadOnly)
		ui.widget->setReadOnly(false);

	int curLine = -1;
	int curIndex = -1;
	ui.widget->getCursorPosition(&curLine, &curIndex);

	int lineFrom = -1, indexFrom = -1, lineTo = -1, indexTo = -1;
	ui.widget->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);

	//紀錄滾動條位置
	int scollValue = ui.widget->verticalScrollBar()->value();

	QTextStream in(&f);
	in.setCodec(util::DEFAULT_CODEPAGE);
	QString c = in.readAll();
	c.replace("\r\n", "\n");

	Injector& injector = Injector::getInstance();
	if (!injector.IS_SCRIPT_FLAG)
		injector.currentScriptFileName = fileName;

	if (!injector.server.isNull() && injector.server->getOnlineFlag())
		setWindowTitle(QString("[%1] %2").arg(injector.server->getPC().name).arg(injector.currentScriptFileName));
	else
		setWindowTitle(injector.currentScriptFileName);

	ui.widget->setUpdatesEnabled(false);

	ui.widget->convertEols(QsciScintilla::EolWindows);
	ui.widget->setUtf8(true);
	ui.widget->setModified(false);
	ui.widget->selectAll();
	ui.widget->replaceSelectedText(c);

	ui.widget->setUpdatesEnabled(true);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.addForwardMarker(-1, false);
	emit signalDispatcher.addErrorMarker(-1, false);
	emit signalDispatcher.addStepMarker(-1, false);

	forward_markers.clear();
	error_markers.clear();
	step_markers.clear();

	reshowBreakMarker();
	emit ui.comboBox_labels->clicked();
	emit ui.comboBox_functions->clicked();

	if (lineFrom >= 0 && indexFrom >= 0 && lineTo >= 0 && indexTo >= 0)
	{
		ui.widget->setSelection(lineFrom, indexFrom, lineTo, indexTo);
	}
	else
	{
		ui.widget->setCursorPosition(curLine, curIndex);
	}

	if (scollValue >= 0)
		ui.widget->verticalScrollBar()->setValue(scollValue);

	if (isReadOnly)
		ui.widget->setReadOnly(true);

	if (injector.IS_SCRIPT_FLAG)
	{
		onScriptStartMode();
	}
}

void ScriptSettingForm::setContinue()
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();

	emit signalDispatcher.addForwardMarker(-1, false);
	emit signalDispatcher.addErrorMarker(-1, false);
	emit signalDispatcher.addStepMarker(-1, false);

	forward_markers.clear();
	error_markers.clear();
	step_markers.clear();

	emit signalDispatcher.scriptResumed();
}

void ScriptSettingForm::stackInfoImport(QTreeWidget* tree, const QVector<QPair<int, QString>>& vec)
{
	if (tree == nullptr)
		return;

	tree->setUpdatesEnabled(false);

	tree->clear();
	tree->setColumnCount(2);
	tree->setHeaderLabels(QStringList() << tr("row") << tr("content"));

	if (!vec.isEmpty())
	{
		QList<QTreeWidgetItem*> trees;
		//int size = d.size();
		for (const QPair<int, QString> pair : vec)
		{
			trees.append(q_check_ptr(new QTreeWidgetItem({ QString::number(pair.first),  pair.second })));
		}

		tree->addTopLevelItems(trees);
	}

	tree->setUpdatesEnabled(true);
}

void ScriptSettingForm::varInfoImport(QTreeWidget* tree, const QHash<QString, QVariant>& d)
{
	if (tree == nullptr)
		return;

	tree->setUpdatesEnabled(false);

	tree->clear();
	tree->setColumnCount(3);
	tree->setHeaderLabels(QStringList() << tr("field") << tr("name") << tr("value") << tr("type"));

	if (!d.isEmpty())
	{
		QMap<QString, QVariant> map;
		QList<QTreeWidgetItem*> trees;

		for (auto it = d.cbegin(); it != d.cend(); ++it)
		{
			map.insert(it.key(), it.value());
		}

		QStringList l;
		for (auto it = map.cbegin(); it != map.cend(); ++it)
		{
			l = it.key().split(util::rexOR);
			if (l.size() != 2)
				continue;

			QString field = l.at(0) == "global" ? tr("GLOBAL") : tr("LOCAL");
			QString varName = l.at(1);
			QString varType;
			QString varValueStr;
			QVariant var = it.value();
			switch (var.type())
			{
			case QVariant::Int:
			{
				varType = tr("Int");
				varValueStr = QString::number(var.toLongLong());
				break;
			}
			case QVariant::UInt:
			{
				varType = tr("UInt");
				varValueStr = QString::number(var.toLongLong());
				break;
			}
			case QVariant::Double:
			{
				varType = tr("Double");
				varValueStr = QString::number(var.toDouble(), 'f', 5);
				break;
			}
			case QVariant::String:
			{
				varValueStr = var.toString();
				if (varValueStr == "nil")
					varType = tr("Nil");
				else if (varValueStr.startsWith("{") && varValueStr.endsWith("}"))
					varType = tr("Table");
				else
					varType = tr("String");
				break;
			}
			case QVariant::Bool:
			{
				varType = tr("Bool");
				varValueStr = var.toBool() ? "true" : "false";
				break;
			}
			case QVariant::LongLong:
			{
				varType = tr("LongLong");
				varValueStr = QString::number(var.toLongLong());
				break;
			}
			case QVariant::ULongLong:
			{
				varType = tr("ULongLong");
				varValueStr = QString::number(var.toULongLong());
				break;
			}
			case QVariant::List:
			{
				varValueStr = var.toString();
				if (varValueStr.startsWith("{") && varValueStr.endsWith("}"))
				{
					varType = tr("Table");

					continue;
				}
				else
					varType = tr("String");
				break;
			}
			default:
			{
				varType = tr("unknown");
				varValueStr = var.toString();
				break;
			}
			}

			trees.append(q_check_ptr(new QTreeWidgetItem({ field, varName, varValueStr, QString("(%1)").arg(varType) })));
		}

		tree->addTopLevelItems(trees);
	}

	tree->setUpdatesEnabled(true);
}

QString ScriptSettingForm::getFullPath(QTreeWidgetItem* item)
{
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

	QFileInfo info(strpath);
	QString suffix = "." + info.suffix();
	if (suffix.isEmpty())
		strpath += util::SCRIPT_DEFAULT_SUFFIX;
	if (suffix != util::SCRIPT_PRIVATE_SUFFIX_DEFAULT && suffix != util::SCRIPT_DEFAULT_SUFFIX)
		strpath.replace(suffix, util::SCRIPT_DEFAULT_SUFFIX);

	return strpath;
};

void ScriptSettingForm::createScriptListContextMenu()
{
	// Create the custom context menu
	QMenu* menu = new QMenu(this);
	QAction* openAcrion = menu->addAction(tr("open"));
	QAction* deleteAction = menu->addAction(tr("delete"));
	QAction* renameAction = menu->addAction(tr("rename"));

	connect(openAcrion, &QAction::triggered, this, [this]()
		{
			//current item
			QTreeWidgetItem* item = ui.treeWidget_scriptList->currentItem();
			if (!item)
				return;

			onScriptTreeWidgetDoubleClicked(item, 0);
		});

	// Connect the menu actions to your slots/functions
	connect(deleteAction, &QAction::triggered, this, [this]()
		{
			//current item
			QTreeWidgetItem* item = ui.treeWidget_scriptList->currentItem();
			if (!item)
				return;

			QString fullpath = getFullPath(item);
			if (fullpath.isEmpty())
				return;


			QFile file(fullpath);
			if (file.exists())
			{
				file.remove();
			}

			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
			emit signalDispatcher.reloadScriptList();
		});

	connect(renameAction, &QAction::triggered, this, [this]()
		{
			//current item
			QTreeWidgetItem* item = ui.treeWidget_scriptList->currentItem();
			if (!item)
				return;

			QString currentText = item->text(0);
			if (currentText.isEmpty())
				return;

			QString currentPath = getFullPath(item);
			if (!QFile::exists(currentPath))
				return;

			//設置為可編輯
			item->setFlags(item->flags() | Qt::ItemIsEditable);

			currentRenamePath_ = currentPath;
			currentRenameText_ = currentText;

			ui.treeWidget_scriptList->editItem(item, 0);
		});

	// Set the context menu policy for the QTreeWidget
	ui.treeWidget_scriptList->setContextMenuPolicy(Qt::CustomContextMenu);

	// Connect the customContextMenuRequested signal to a slot
	connect(ui.treeWidget_scriptList, &QTreeWidget::customContextMenuRequested, this, [this, menu](const QPoint& pos)
		{
			QTreeWidgetItem* item = ui.treeWidget_scriptList->itemAt(pos);
			if (item)
			{
				// Show the context menu at the requested position
				menu->exec(ui.treeWidget_scriptList->mapToGlobal(pos));

				SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
				emit signalDispatcher.reloadScriptList();
			}
		});
}

void ScriptSettingForm::setMark(CodeEditor::SymbolHandler element, util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>>& hash, int liner, bool b)
{
	do
	{
		if (liner == -1)
		{
			hash.clear();
			ui.widget->markerDeleteAll(element);
			break;
		}

		Injector& injector = Injector::getInstance();
		util::SafeHash<qint64, break_marker_t> markers = hash.value(injector.currentScriptFileName);

		if (b)
		{
			break_marker_t bk = markers.value(liner);
			bk.line = liner;
			bk.content = ui.widget->text(liner);
			bk.maker = static_cast<qint64>(element);
			markers.insert(liner, bk);
			hash.insert(injector.currentScriptFileName, markers);
			ui.widget->markerAdd(liner, element);
			emit editorCursorPositionChanged(liner, 0);
		}
		else
		{
			if (markers.contains(liner))
			{
				markers.remove(liner);
				hash.insert(injector.currentScriptFileName, markers);
				ui.widget->markerDelete(liner, element);
			}
		}
	} while (false);
}

void ScriptSettingForm::setStepMarks()
{
	QString content = ui.widget->text();
	QStringList list = content.split("\n");
	if (list.isEmpty())
		return;

	int maxliner = list.size();
	int index = 1;

	Injector& injector = Injector::getInstance();
	util::SafeHash<qint64, break_marker_t> markers = step_markers.value(injector.currentScriptFileName);
	break_marker_t bk = {};

	for (int i = 0; i < maxliner; ++i)
	{
		index = i + 1;
		bk = markers.value(index);
		bk.line = index;
		bk.content = list.at(i);
		bk.maker = static_cast<qint64>(CodeEditor::SymbolHandler::SYM_STEP);
		markers.insert(index, bk);
	}

	step_markers.insert(injector.currentScriptFileName, markers);
}

void ScriptSettingForm::reshowBreakMarker()
{
	Injector& injector = Injector::getInstance();
	const util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>> mks = break_markers;
	for (auto it = mks.cbegin(); it != mks.cend(); ++it)
	{
		QString fileName = it.key();
		if (fileName != injector.currentScriptFileName)
			continue;

		const util::SafeHash<qint64, break_marker_t> mk = mks.value(fileName);
		for (const break_marker_t& bit : mk)
		{
			ui.widget->markerAdd(bit.line, CodeEditor::SymbolHandler::SYM_POINT);
		}
	}

	emit breakMarkInfoImport();
}

QString formatScriptLine(const QString& inputLine)
{
	//return formattedLine;
	QString formattedLine = inputLine;
	QRegularExpression regex("\\{([^\\s'\"].*?[^\\s'\"])\\}");
	formattedLine.replace(regex, "{ \\1 }");

	return formattedLine;
}

//////////////////////

void ScriptSettingForm::on_widget_marginClicked(int margin, int line, Qt::KeyboardModifiers state)
{
	Q_UNUSED(margin);
	int mask = ui.widget->markersAtLine(line);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	switch (state)
	{
	case Qt::ControlModifier: // 按下Ctrl键
	case Qt::AltModifier: break;// 按下Alt键
	default:
	{
		if (ui.widget->isBreak(mask))
		{
			emit signalDispatcher.addBreakMarker(line, false);
		}
		else
		{
			emit signalDispatcher.addBreakMarker(line, true);
		}
		break;
	}
	}
}

void ScriptSettingForm::on_widget_cursorPositionChanged(int line, int index)
{
	const QsciScintilla::EolMode mode = ui.widget->eolMode();
	const QString modeStr(mode == QsciScintilla::EolWindows ? "CRLF" : mode == QsciScintilla::EolUnix ? "LF" : "CR");
	//獲取當前行字
	const QString lineText = ui.widget->text(line);
	staticLabel_.setText(QString(tr("row:%1 | size:%2 | index:%3 | %4").arg(line + 1).arg(lineText.size()).arg(index).arg(modeStr)));
}

void ScriptSettingForm::on_widget_textChanged()
{
	Injector& injector = Injector::getInstance();
	const QString text(ui.widget->text());
	if (scripts_.value(injector.currentScriptFileName, "") != text)
	{
		scripts_.insert(injector.currentScriptFileName, text);
		isModified_ = true;
		ui.widget->setModified(true);
		emit ui.widget->modificationChanged(true);
	}
	else
	{
		isModified_ = false;
		ui.widget->setModified(false);
	}
}

void ScriptSettingForm::on_comboBox_labels_clicked()
{
	static const QRegularExpression rex_divLabel(R"((label\s+[\p{Han}\w]+))");
	ui.comboBox_labels->blockSignals(true);

	//將所有lua function 和 label加入 combobox並使其能定位行號
	int line = -1, index = -1;
	ui.widget->getCursorPosition(&line, &index);

	const QString contents = ui.widget->text();
	const QStringList conlist = contents.split("\n");
	int cur = ui.comboBox_labels->currentIndex();
	ui.comboBox_labels->clear();

	int count = conlist.size();
	for (int i = 0; i < count; ++i)
	{
		const QString linestr = conlist.at(i).simplified();
		if (linestr.isEmpty())
			continue;
		QRegularExpressionMatchIterator it = rex_divLabel.globalMatch(linestr);
		while (it.hasNext())
		{
			const QRegularExpressionMatch match = it.next();
			const QString str = match.captured(1).simplified();
			if (!str.isEmpty())
			{
				ui.comboBox_labels->addItem(QIcon(":/image/icon_label.png"), str.simplified(), QVariant(i));
			}
		}
	}

	if (cur < 0 || cur >= ui.comboBox_functions->count())
		cur = 0;

	ui.comboBox_labels->setCurrentIndex(cur);

	ui.comboBox_labels->blockSignals(false);
}

//標記列表點擊
void ScriptSettingForm::on_comboBox_labels_currentIndexChanged(int)
{
	QVariant var = ui.comboBox_labels->currentData();
	if (var.isValid())
	{
		int line = var.toInt();
		emit editorCursorPositionChanged(line, NULL);
	}
}

void ScriptSettingForm::on_comboBox_functions_clicked()
{
	static const QRegularExpression rex_divLabel(R"((function\s+[\p{Han}\w]+))");
	ui.comboBox_functions->blockSignals(true);
	//將所有lua function 和 label加入 combobox並使其能定位行號
	int line = -1, index = -1;
	ui.widget->getCursorPosition(&line, &index);

	const QString contents = ui.widget->text();
	const QStringList conlist = contents.split("\n");
	int cur = ui.comboBox_functions->currentIndex();
	ui.comboBox_functions->clear();

	int count = conlist.size();
	for (int i = 0; i < count; ++i)
	{
		const QString linestr = conlist.at(i).simplified();
		if (linestr.isEmpty())
			continue;
		QRegularExpressionMatchIterator it = rex_divLabel.globalMatch(linestr);
		while (it.hasNext())
		{
			const QRegularExpressionMatch match = it.next();
			const QString str = match.captured(1).simplified();
			if (!str.isEmpty())
			{
				ui.comboBox_functions->addItem(QIcon(":/image/icon_func.png"), str.simplified(), QVariant(i));
			}
		}
	}

	if (cur < 0 || cur >= ui.comboBox_functions->count())
		cur = 0;

	ui.comboBox_functions->setCurrentIndex(cur);
	ui.comboBox_functions->blockSignals(false);
}

void ScriptSettingForm::on_comboBox_functions_currentIndexChanged(int)
{
	QVariant var = ui.comboBox_functions->currentData();
	if (var.isValid())
	{
		int line = var.toInt();
		emit editorCursorPositionChanged(line, NULL);
	}
}

void ScriptSettingForm::on_treeWidget_functionList_itemDoubleClicked(QTreeWidgetItem* item, int column)
{
	Q_UNUSED(column);
	//insert selected item text to widget

	if (!item)
		return;

	//如果存在子節點則直接返回
	if (item->childCount() > 0)
		return;

	QString str = item->text(0);
	if (str.isEmpty())
		return;

	Injector& injector = Injector::getInstance();

	if (injector.server.isNull())
	{
		ui.widget->insert(str + " ");
		return;
	}

	if (str == "button" || str == "input")
	{
		dialog_t dialog = injector.server->currentDialog;
		QString npcName = injector.server->mapUnitHash.value(dialog.unitid).name;
		if (npcName.isEmpty())
			return;

		str = QString("%1 '', '%2', %3").arg(str).arg(npcName).arg(dialog.dialogid);
	}
	else if (str == "waitdlg")
	{
		dialog_t dialog = injector.server->currentDialog;
		QString lineStr;
		for (const QString& s : dialog.linedatas)
		{
			if (s.size() > 3)
			{
				lineStr = s;
				break;
			}
		}
		str = QString("%1 '%2', ?, 5000, -1").arg(str).arg(lineStr);
	}
	else if (str == "learn")
	{
		dialog_t dialog = injector.server->currentDialog;
		QString npcName = injector.server->mapUnitHash.value(dialog.unitid).name;
		if (npcName.isEmpty())
			return;

		str = QString("%1 0, 0, 0, '%2', %3").arg(str).arg(npcName).arg(dialog.dialogid);
	}
	else if (str == "findpath" || str == "move")
	{
		QPoint pos = injector.server->getPoint();
		str = QString("%1 %2, %3").arg(str).arg(pos.x()).arg(pos.y());
	}
	else if (str == "dir")
	{
		int dir = (injector.server->getPC().dir + 3) % 8;
		str = QString("%1 %2").arg(str).arg(dir);
	}
	else if (str == "walkpos")
	{
		QPoint pos = injector.server->getPoint();
		str = QString("%1 %2, %3, 5000").arg(str).arg(pos.x()).arg(pos.y());
	}
	else if (str == "w")
	{
		QPoint pos = injector.server->getPoint();
		int dir = (injector.server->getPC().dir + 3) % 8;
		const QString dirStr = "ABCDEFGH";
		if (dir < 0 || dir >= dirStr.size())
			return;
		str = QString("%1 %2, %3, '%4'").arg(str).arg(pos.x()).arg(pos.y()).arg(dirStr.at(dir));
	}
	else if (str == "ifpos")
	{
		QPoint pos = injector.server->getPoint();
		str = QString("%1 %2, %3, +2").arg(str).arg(pos.x()).arg(pos.y());
	}
	else if (str == "ifmap")
	{
		int floor = injector.server->nowFloor;
		str = QString("%1 %2, +2").arg(str).arg(floor);
	}
	else if (str == "waitmap")
	{
		int floor = injector.server->nowFloor;
		str = QString("%1 %2, 5000, +2").arg(str).arg(floor);
	}
	else if (str == "sleep")
	{
		str = QString("%1 500").arg(str);
	}
	else if (str == "function")
	{
		str = QString("%1 myfun()\n    \nend").arg(str);
	}
	else if (str == "for")
	{
		str = QString("%1 i = 1, 10, 1\n    \nend").arg(str);
	}
	else if (str == "movetonpc")
	{
		QPoint pos = injector.server->getPoint();
		QList<mapunit_t> units = injector.server->mapUnitHash.values();
		auto getdis = [&pos](QPoint p)
		{
			//歐幾里得
			return sqrt(pow(pos.x() - p.x(), 2) + pow(pos.y() - p.y(), 2));
		};

		std::sort(units.begin(), units.end(), [&getdis](const mapunit_t& a, const mapunit_t& b)
			{
				return getdis(a.p) < getdis(b.p);
			});

		if (units.isEmpty())
			return;
		mapunit_t unit = units.first();
		while (unit.objType != util::OBJ_NPC)
		{
			units.removeFirst();
			if (units.isEmpty())
				return;
			unit = units.first();
		}

		//移动至NPC 参数1:NPC名称, 参数2:NPC暱称, 参数3:东坐标, 参数4:南坐标, 参数5:超时时间, 参数6:错误跳转
		str = QString("%1 '%2', '%3', %4, %5, 10000, -1").arg(str).arg(unit.name).arg(unit.freeName).arg(unit.p.x()).arg(unit.p.y());

	}
	else if (str == "movetonpc with mod")
	{
		QPoint pos = injector.server->getPoint();
		QList<mapunit_t> units = injector.server->mapUnitHash.values();
		auto getdis = [&pos](QPoint p)
		{
			//歐幾里得
			return sqrt(pow(pos.x() - p.x(), 2) + pow(pos.y() - p.y(), 2));
		};

		std::sort(units.begin(), units.end(), [&getdis](const mapunit_t& a, const mapunit_t& b)
			{
				return getdis(a.p) < getdis(b.p);
			});

		if (units.isEmpty())
			return;
		mapunit_t unit = units.first();
		while (unit.objType != util::OBJ_NPC)
		{
			units.removeFirst();
			if (units.isEmpty())
				return;
			unit = units.first();
		}

		//移动至NPC 参数1:NPC名称, 参数2:NPC暱称, 参数3:东坐标, 参数4:南坐标, 参数5:超时时间, 参数6:错误跳转
		str = QString("movetonpc %1, '', %2, %3, 10000, -1").arg(unit.modelid).arg(unit.p.x()).arg(unit.p.y());
	}
	ui.widget->insert(str);
}

void ScriptSettingForm::on_treeWidget_functionList_itemClicked(QTreeWidgetItem*, int)
{
	emit ui.treeWidget_functionList->itemSelectionChanged();
}

void ScriptSettingForm::on_treeWidget_functionList_itemSelectionChanged()
{
	do
	{
		QTreeWidgetItem* item = ui.treeWidget_functionList->currentItem();
		if (item == nullptr)
			break;

		QString str = item->text(0);
		if (str.isEmpty())
			break;

		if (document_.contains(str))
		{
			QSharedPointer< QTextDocument> doc = document_.value(str);
			ui.textBrowser->setUpdatesEnabled(false);
			ui.textBrowser->setDocument(doc.data());
			ui.textBrowser->setUpdatesEnabled(true);
			break;
		}
#ifdef _DEBUG
		QString mdFullPath = R"(../Debug/lib/doc)";
#else
		QString mdFullPath = QString("%1/lib/doc").arg(util::applicationDirPath());
#endif
		QDir dir(mdFullPath);
		if (!dir.exists())
			break;

		QStringList result;
		//以此目錄為起點遍歷查找所有文件名包含 \'str\' 的 .MD 文件
		util::searchFiles(mdFullPath, QString(R"('%1')").arg(str), ".md", &result);
		if (result.isEmpty())
			return;

		//按文本長度排序 由短到長
		std::sort(result.begin(), result.end(), [](const QString& a, const QString& b)
			{
				return a.length() < b.length();
			});

		QString markdownText = result.join("\n---\n");


		QSharedPointer< QTextDocument> doc = QSharedPointer<QTextDocument>(new QTextDocument());

		doc->setMarkdown(markdownText);
		document_.insert(str, doc);

		ui.textBrowser->setUpdatesEnabled(false);
		ui.textBrowser->setDocument(doc.data());
		ui.textBrowser->setUpdatesEnabled(true);

		return;
	} while (false);

}

void ScriptSettingForm::on_treeWidget_scriptList_itemClicked(QTreeWidgetItem* item, int column)
{
	if (column != 0 || !item || item->childCount() > 0)
		return;

	QString str = item->text(0);
	if (str.isEmpty())
		return;

	// 獲取項目的邊界框
	QRect itemRect = ui.treeWidget_scriptList->visualItemRect(item);

	// 獲取項目的文字區域邊界框
	QFontMetrics fontMetrics(ui.treeWidget_scriptList->font());
	QRect textRect = fontMetrics.boundingRect(itemRect, Qt::TextSingleLine, str);

	// 將文字區域邊界框擴展一些，以提供一個較大的可點擊範圍
	int padding = 20; // 可以根據需要調整
	textRect.adjust(-padding, -padding, padding, padding);

	// 檢查滑鼠位置是否在文字範圍內
	QPoint pos = ui.treeWidget_scriptList->mapFromGlobal(QCursor::pos());
	if (!textRect.contains(pos))
		return;

	ui.treeWidget_scriptList->setCurrentItem(item);


	QString currentText = item->text(0);
	if (currentText.isEmpty())
		return;

	QString currentPath = getFullPath(item);
	if (!QFile::exists(currentPath))
		return;

	//設置為可編輯
	item->setFlags(item->flags() | Qt::ItemIsEditable);

	currentRenamePath_ = currentPath;
	currentRenameText_ = currentText;

	ui.treeWidget_scriptList->editItem(item, 0);
}

void ScriptSettingForm::on_treeWidget_breakList_itemDoubleClicked(QTreeWidgetItem* item, int column)
{
	Q_UNUSED(column);
	if (item == nullptr)
		return;
	Injector& injector = Injector::getInstance();
	if (item->text(2).isEmpty()) return;

	if (!item->text(3).isEmpty() && item->text(3) == injector.currentScriptFileName)
	{
		int line = item->text(2).toInt();
		//ui.widget->setCursorPosition(line, 0);
		QString text = ui.widget->text(line - 1);
		ui.widget->setSelection(line - 1, 0, line - 1, text.length());
		ui.widget->ensureLineVisible(line - 1);
	}
}

//查找命令
void ScriptSettingForm::on_lineEdit_searchFunction_textChanged(const QString& text)
{
	static const auto OnFindItem = [](QTreeWidget* tree, const QString& qsFilter)->void
	{
		if (!tree) return;
		QTreeWidgetItemIterator it(tree);
		while (*it)
		{
			//QTreeWidgetItem是否滿足條件---這里的條件可以自己修改
			if ((*it)->text(0).contains(qsFilter))
			{
				(*it)->setExpanded(true);
				(*it)->setHidden(false);
				QTreeWidgetItem* item = *it;
				//顯示父節點
				while (item->parent())
				{
					item->parent()->setHidden(false);
					item = item->parent();
					//展開
					item->setExpanded(true);
				}
			}
			else
			{
				//不滿足滿足條件先隱藏，它的子項目滿足條件時會再次讓它顯示
				(*it)->setHidden(true);
			}
			++it;
		}
	};

	if (!text.isEmpty())
		OnFindItem(ui.treeWidget_functionList, text);
	else
	{
		QTreeWidgetItemIterator it(ui.treeWidget_functionList);
		while (*it)
		{
			(*it)->setHidden(false);
			++it;
		}
	}
}

void ScriptSettingForm::on_listView_log_doubleClicked(const QModelIndex& index)
{
	QString text = index.data().toString();
	if (text.isEmpty())
		return;

	// "於行號: 1" | "于行号: 1"  取最後 : 後面的數字
	static const QRegularExpression re("@\\s*(\\d+)\\s*\\|");
	QRegularExpressionMatch match = re.match(text);
	if (match.hasMatch())
	{
		QString line = match.captured(1);
		if (line.isEmpty())
			line = match.captured(2);

		if (!line.isEmpty())
		{
			//ui.widget->setCursorPosition(line.toInt(), 0);
			text = ui.widget->text(line.toInt() - 1);
			ui.widget->setSelection(line.toInt() - 1, 0, line.toInt() - 1, text.length());
			ui.widget->ensureLineVisible(line.toInt() - 1);
			return;
		}
	}

	//[%1 | @%2]: %3 取@後面的數字
	static const QRegularExpression re2(R"(\[\d{1,2}\:\d{1,2}\:\d{1,2}\:\d{1,3}\s+\|\s+@\s*(\d+)\])");
	match = re2.match(text);
	if (match.hasMatch())
	{
		QString line = match.captured(1);
		if (!line.isEmpty())
		{
			//ui.widget->setCursorPosition(line.toInt(), 0);
			text = ui.widget->text(line.toInt() - 1);
			ui.widget->setSelection(line.toInt() - 1, 0, line.toInt() - 1, text.length());
			ui.widget->ensureLineVisible(line.toInt() - 1);
			return;
		}
	}
}

void ScriptSettingForm::onApplyHashSettingsToUI()
{
	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull() && injector.server->getOnlineFlag())
	{
		QString title = injector.currentScriptFileName;
		QString newTitle = QString("[%1] %2").arg(injector.server->getPC().name).arg(title);
		setWindowTitle(newTitle);
	}

	if (!injector.scriptLogModel.isNull())
		ui.listView_log->setModel(injector.scriptLogModel.get());

	pSpeedSpinBox->setValue(injector.getValueHash(util::kScriptSpeedValue));

	ui.actionDebug->setChecked(injector.isScriptDebugModeEnable.load(std::memory_order_acquire));
}

void ScriptSettingForm::onWidgetModificationChanged(bool changed)
{
	if (!changed) return;
	Injector& injector = Injector::getInstance();
	scripts_.insert(injector.currentScriptFileName, ui.widget->text());
}

void ScriptSettingForm::onScriptTreeWidgetDoubleClicked(QTreeWidgetItem* item, int column)
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
		emit signalDispatcher.addForwardMarker(-1, false);
		emit signalDispatcher.addErrorMarker(-1, false);
		emit signalDispatcher.addStepMarker(-1, false);

		forward_markers.clear();
		error_markers.clear();
		step_markers.clear();

		emit signalDispatcher.loadFileToTable(strpath);

	} while (false);

	IS_LOADING = false;
}

void ScriptSettingForm::onScriptTreeWidgetHeaderClicked(int logicalIndex)
{
	if (logicalIndex == 0)
	{
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.reloadScriptList();
	}
}

void ScriptSettingForm::onScriptTreeWidgetItemChanged(QTreeWidgetItem* newitem, int column)
{
	if (column != 0 || !newitem)
		return;

	QString newtext = newitem->text(0);
	if (newtext.isEmpty())
	{
		return;
	}

	if (newtext == currentRenameText_)
		return;

	QString str = getFullPath(newitem);
	if (str.isEmpty())
		return;

	if (str == currentRenamePath_)
		return;

	QFile file(currentRenamePath_);
	if (file.exists() && !QFile::exists(str))
	{
		file.rename(str);
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.reloadScriptList();
}

void ScriptSettingForm::onScriptLabelRowTextChanged(int line, int, bool)
{
	Injector& injector = Injector::getInstance();
	if (!injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
		return;

	if (line < 0)
		line = 0;

	ui.widget->setUpdatesEnabled(false);
	QString text = ui.widget->text(line - 1);
	ui.widget->setSelection(line - 1, 0, line - 1, text.size());
	ui.widget->setCaretLineBackgroundColor(QColor(71, 71, 71));//光標所在行背景顏色
	ui.widget->setUpdatesEnabled(true);

	//確保光標示在可視範圍內
	ui.widget->ensureLineVisible(line - 1);
}

//切換光標和焦點所在行
void ScriptSettingForm::onEditorCursorPositionChanged(int line, int index)
{
	//ui.widget->setCursorPosition(line, ui.widget->SendScintilla(QsciScintilla::SCI_LINELENGTH, line - 1) - 1);
	//ui.widget->SendScintilla(QsciScintilla::SCI_SETSELECTIONMODE, QsciScintilla::SC_SEL_LINES);

	if (NULL == index)
		ui.widget->setCaretLineBackgroundColor(QColor(71, 71, 71));//光標所在行背景顏色
	else
		ui.widget->setCaretLineBackgroundColor(QColor(0xff, 0x80, 0x80));//光標所在行背景顏色

	//確保光標示在可視範圍內
	if (line - 1 < 0)
		line = 1;
	ui.widget->ensureLineVisible(line - 1);
}

void ScriptSettingForm::onSpeedChanged(int value)
{
	pSpeedSpinBox->blockSignals(true);
	pSpeedSpinBox->setValue(value);
	pSpeedSpinBox->blockSignals(false);

	Injector& injector = Injector::getInstance();
	injector.setValueHash(util::kScriptSpeedValue, value);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.applyHashSettingsToUI();
}

void ScriptSettingForm::onActionTriggered()
{
	QAction* pAction = qobject_cast<QAction*>(sender());
	if (!pAction)
		return;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	QString name = pAction->objectName();
	if (name.isEmpty())
		return;

	Injector& injector = Injector::getInstance();

	if (name == "actionSave")
	{
		fileSave(ui.widget->text(), QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
		emit signalDispatcher.loadFileToTable(injector.currentScriptFileName);
		emit signalDispatcher.reloadScriptList();
	}
	else if (name == "actionStart")
	{
		//Injector& injector = Injector::getInstance();
		if (step_markers.size() == 0 && !injector.IS_SCRIPT_FLAG.load(std::memory_order_acquire) && QFile::exists(injector.currentScriptFileName))
		{
			emit signalDispatcher.scriptStarted();
		}
	}
	else if (name == "actionPause")
	{
		emit signalDispatcher.scriptPaused();
	}
	else if (name == "actionStop")
	{
		emit signalDispatcher.scriptStoped();
	}
	else if (name == "actionContinue")
	{
		setContinue();
	}
	else if (name == "actionStep")
	{
		emit signalDispatcher.addForwardMarker(-1, false);
		emit signalDispatcher.addErrorMarker(-1, false);
		emit signalDispatcher.scriptBreaked();
	}
	else if (name == "actionSaveAs")
	{
		const QString directoryName(util::applicationDirPath() + "/script");
		const QDir dir(directoryName);
		if (!dir.exists())
			dir.mkdir(directoryName);

		QFileDialog fd(this, tr("saveas"), directoryName, "Text File(*.txt)");
		fd.setAttribute(Qt::WA_QuitOnClose);
		fd.setAcceptMode(QFileDialog::AcceptSave);
		fd.setViewMode(QFileDialog::Detail); //詳細
		fd.setFileMode(QFileDialog::AnyFile);
		fd.setDefaultSuffix("txt");
		fd.setOption(QFileDialog::DontUseNativeDialog, false);
		fd.setWindowModality(Qt::WindowModal);
		if (fd.exec() == QDialog::Accepted)
		{
			const QString f(fd.selectedFiles().at(0));
			if (f.isEmpty()) return;

			QFile file(f);
			if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
			{
				QTextStream ts(&file);
				ts.setCodec(util::DEFAULT_CODEPAGE);
				ts.setGenerateByteOrderMark(true);
				ts << ui.widget->text() << Qt::endl;
				file.flush();
				file.close();
				QDesktopServices::openUrl(QUrl::fromLocalFile(directoryName));
			}
		}
		emit signalDispatcher.reloadScriptList();
	}
	else if (name == "actionDirectory")
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(util::applicationDirPath() + "/script"));
	}
	else if (name == "actionMark")
	{
		int line = -1;
		int index = -1;
		ui.widget->getCursorPosition(&line, &index);
		emit ui.widget->marginClicked(NULL, line, Qt::NoModifier);
	}
	else if (name == "actionLogback")
	{
		//Injector& injector = Injector::getInstance();
		if (injector.server.isNull())
			return;

		injector.server->logBack();
	}
	else if (name == "actionAutoFollow")
	{

	}
	else if (name == "actionNew")
	{
		int num = 1;
		for (;;)
		{
			QString strpath = (util::applicationDirPath() + QString("/script/Untitled-%1.txt").arg(num));
			if (!QFile::exists(strpath))
			{
				QFile file(strpath);
				if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
				{
					QTextStream ts(&file);
					ts.setCodec(util::DEFAULT_CODEPAGE);
					ts.setGenerateByteOrderMark(true);
					ts << "" << Qt::endl;
					file.flush();
					file.close();
				}

				loadFile(strpath);

				emit signalDispatcher.reloadScriptList();
				break;
			}
			++num;
		}

	}
	else if (name == "actionSaveEncode")
	{
		onEncryptSave();
	}
	else if (name == "actionSaveDecode")
	{
		onDecryptSave();
	}
	else if (name == "actionDebug")
	{
		injector.isScriptDebugModeEnable.store(pAction->isChecked(), std::memory_order_release);
	}
}

void ScriptSettingForm::onScriptStartMode()
{
	ui.mainToolBar->setUpdatesEnabled(false);

	ui.mainToolBar->clear();
	ui.actionLogback->setEnabled(false);
	ui.actionSave->setEnabled(false);
	ui.actionNew->setEnabled(false);
	ui.actionSaveAs->setEnabled(false);
	ui.actionDirectory->setEnabled(true);
	ui.mainToolBar->addAction(ui.actionLogback);
	ui.mainToolBar->addAction(ui.actionSave);
	ui.mainToolBar->addAction(ui.actionNew);
	ui.mainToolBar->addAction(ui.actionSaveAs);
	ui.mainToolBar->addAction(ui.actionDirectory);

	ui.mainToolBar->addSeparator();

	ui.actionStart->setEnabled(false);
	ui.actionStop->setEnabled(true);
	ui.actionPause->setEnabled(true);
	ui.actionStop->setIcon(QIcon(":/image/icon_stop.png"));
	ui.actionPause->setIcon(QIcon(":/image/icon_pause.png"));
	ui.actionPause->setText(tr("pause"));

	ui.mainToolBar->addAction(ui.actionStart);
	ui.mainToolBar->addAction(ui.actionStop);
	ui.mainToolBar->addAction(ui.actionPause);

	ui.mainToolBar->addSeparator();

	ui.actionStep->setEnabled(false);
	ui.actionMark->setEnabled(true);

	//ui.mainToolBar->addAction(ui.actionStep);
	ui.mainToolBar->addAction(ui.actionMark);

	ui.mainToolBar->addSeparator();

	ui.mainToolBar->addAction(ui.actionDebug);

	ui.mainToolBar->addSeparator();

	createSpeedSpinBox();

	//禁止編輯
	ui.widget->setReadOnly(true);

	ui.mainToolBar->setUpdatesEnabled(true);
}

void ScriptSettingForm::onScriptStopMode()
{
	ui.mainToolBar->setUpdatesEnabled(false);

	ui.mainToolBar->clear();
	ui.actionLogback->setEnabled(true);
	ui.actionSave->setEnabled(true);
	ui.actionNew->setEnabled(true);
	ui.actionSaveAs->setEnabled(true);
	ui.actionDirectory->setEnabled(true);
	ui.mainToolBar->addAction(ui.actionLogback);
	ui.mainToolBar->addAction(ui.actionSave);
	ui.mainToolBar->addAction(ui.actionNew);
	ui.mainToolBar->addAction(ui.actionSaveAs);
	ui.mainToolBar->addAction(ui.actionDirectory);

	ui.mainToolBar->addSeparator();

	ui.actionStart->setEnabled(true);
	ui.actionStop->setEnabled(false);
	ui.actionPause->setEnabled(false);
	ui.actionStop->setIcon(QIcon(":/image/icon_stop_diaable.png"));
	ui.actionPause->setIcon(QIcon(":/image/icon_pause_diaable.png"));
	ui.actionPause->setText(tr("pause"));

	ui.mainToolBar->addAction(ui.actionStart);
	//ui.mainToolBar->addAction(ui.actionStop);
	//ui.mainToolBar->addAction(ui.actionPause);

	ui.mainToolBar->addSeparator();

	ui.actionStep->setEnabled(false);
	ui.actionMark->setEnabled(true);

	//ui.mainToolBar->addAction(ui.actionStep);
	ui.mainToolBar->addAction(ui.actionMark);

	ui.mainToolBar->addSeparator();

	ui.actionSaveEncode->setEnabled(true);
	ui.actionSaveDecode->setEnabled(true);

	ui.mainToolBar->addAction(ui.actionSaveEncode);
	ui.mainToolBar->addAction(ui.actionSaveDecode);

	ui.mainToolBar->addSeparator();

	ui.mainToolBar->addAction(ui.actionDebug);

	ui.mainToolBar->addSeparator();

	createSpeedSpinBox();

	ui.widget->setReadOnly(false);

	ui.mainToolBar->setUpdatesEnabled(true);

	onAddForwardMarker(-1, false);
	onAddStepMarker(-1, false);
	forward_markers.clear();
	step_markers.clear();
}

void ScriptSettingForm::onScriptBreakMode()
{
	ui.mainToolBar->setUpdatesEnabled(false);

	ui.mainToolBar->clear();
	ui.actionLogback->setEnabled(false);
	ui.actionSave->setEnabled(false);
	ui.actionNew->setEnabled(false);
	ui.actionSaveAs->setEnabled(false);
	ui.actionDirectory->setEnabled(true);
	ui.mainToolBar->addAction(ui.actionLogback);
	ui.mainToolBar->addAction(ui.actionSave);
	ui.mainToolBar->addAction(ui.actionNew);
	ui.mainToolBar->addAction(ui.actionSaveAs);
	ui.mainToolBar->addAction(ui.actionDirectory);

	ui.mainToolBar->addSeparator();

	ui.actionStart->setEnabled(false);
	ui.actionContinue->setEnabled(true);
	ui.actionStop->setEnabled(true);
	ui.actionPause->setEnabled(false);
	ui.actionStop->setIcon(QIcon(":/image/icon_stop.png"));
	ui.actionPause->setIcon(QIcon(":/image/icon_pause_disable.png"));

	//ui.mainToolBar->addAction(ui.actionStart);
	ui.mainToolBar->addAction(ui.actionContinue);
	ui.mainToolBar->addAction(ui.actionStop);
	ui.mainToolBar->addAction(ui.actionPause);

	ui.mainToolBar->addSeparator();

	ui.actionStep->setEnabled(true);
	ui.actionMark->setEnabled(true);

	ui.mainToolBar->addAction(ui.actionStep);
	ui.mainToolBar->addAction(ui.actionMark);

	ui.mainToolBar->addSeparator();

	ui.mainToolBar->addAction(ui.actionDebug);

	ui.mainToolBar->addSeparator();

	createSpeedSpinBox();

	//禁止編輯
	ui.widget->setReadOnly(true);

	ui.mainToolBar->setUpdatesEnabled(true);
}

void ScriptSettingForm::onScriptPauseMode()
{
	ui.mainToolBar->setUpdatesEnabled(false);

	ui.mainToolBar->clear();
	ui.actionLogback->setEnabled(false);
	ui.actionSave->setEnabled(false);
	ui.actionNew->setEnabled(false);
	ui.actionSaveAs->setEnabled(false);
	ui.actionDirectory->setEnabled(true);
	ui.mainToolBar->addAction(ui.actionLogback);
	ui.mainToolBar->addAction(ui.actionSave);
	ui.mainToolBar->addAction(ui.actionNew);
	ui.mainToolBar->addAction(ui.actionSaveAs);
	ui.mainToolBar->addAction(ui.actionDirectory);

	ui.mainToolBar->addSeparator();

	ui.actionStart->setEnabled(false);
	ui.actionContinue->setEnabled(true);
	ui.actionStop->setEnabled(true);
	ui.actionPause->setEnabled(false);
	ui.actionStop->setIcon(QIcon(":/image/icon_stop.png"));
	ui.actionPause->setIcon(QIcon(":/image/icon_pause_disable.png"));

	//ui.mainToolBar->addAction(ui.actionStart);
	ui.mainToolBar->addAction(ui.actionContinue);
	ui.mainToolBar->addAction(ui.actionStop);
	ui.mainToolBar->addAction(ui.actionPause);

	ui.mainToolBar->addSeparator();

	ui.actionStep->setEnabled(true);
	ui.actionMark->setEnabled(true);

	ui.mainToolBar->addAction(ui.actionStep);
	ui.mainToolBar->addAction(ui.actionMark);

	ui.mainToolBar->addSeparator();

	ui.mainToolBar->addAction(ui.actionDebug);

	ui.mainToolBar->addSeparator();

	createSpeedSpinBox();

	//禁止編輯
	ui.widget->setReadOnly(true);

	ui.mainToolBar->setUpdatesEnabled(true);
}

void ScriptSettingForm::onAddForwardMarker(int liner, bool b)
{
	setMark(CodeEditor::SymbolHandler::SYM_ARROW, forward_markers, liner, b);
}

void ScriptSettingForm::onAddErrorMarker(int liner, bool b)
{
	setMark(CodeEditor::SymbolHandler::SYM_TRIANGLE, error_markers, liner, b);
}

void ScriptSettingForm::onAddStepMarker(int, bool b)
{
	if (!b)
	{
		ui.widget->setUpdatesEnabled(false);
		setMark(CodeEditor::SymbolHandler::SYM_STEP, step_markers, -1, false);
		ui.widget->setUpdatesEnabled(true);
	}
	else
	{
		ui.widget->setUpdatesEnabled(false);
		setStepMarks();
		ui.widget->setUpdatesEnabled(true);
		emit SignalDispatcher::getInstance().scriptBreaked();
	}
}

void ScriptSettingForm::onAddBreakMarker(int liner, bool b)
{
	Injector& injector = Injector::getInstance();
	do
	{
		if (liner == -1)
		{
			ui.widget->markerDeleteAll(CodeEditor::SymbolHandler::SYM_POINT);
			break;
		}

		if (b)
		{
			util::SafeHash<qint64, break_marker_t> markers = break_markers.value(injector.currentScriptFileName);
			break_marker_t bk = markers.value(liner);
			bk.line = liner;
			bk.content = ui.widget->text(liner);
			if (bk.content.simplified().isEmpty() || bk.content.simplified().indexOf("//") == 0 || bk.content.simplified().indexOf("/*") == 0)
				return;

			bk.maker = static_cast<qint64>(CodeEditor::SymbolHandler::SYM_POINT);

			markers.insert(liner, bk);
			break_markers.insert(injector.currentScriptFileName, markers);
			ui.widget->markerAdd(liner, CodeEditor::SymbolHandler::SYM_POINT);
		}
		else if (!b)
		{
			util::SafeHash<qint64, break_marker_t> markers = break_markers.value(injector.currentScriptFileName);
			if (markers.contains(liner))
			{
				markers.remove(liner);
				break_markers.insert(injector.currentScriptFileName, markers);
			}

			ui.widget->markerDelete(liner, CodeEditor::SymbolHandler::SYM_POINT);
		}
	} while (false);

	emit breakMarkInfoImport();
}

void ScriptSettingForm::onBreakMarkInfoImport()
{
	ui.treeWidget_breakList->setUpdatesEnabled(false);

	QList<QTreeWidgetItem*> trees = {};
	ui.treeWidget_breakList->clear();
	ui.treeWidget_breakList->setColumnCount(4);
	ui.treeWidget_breakList->setHeaderLabels(QStringList{ tr("CONTENT"),tr("COUNT"), tr("ROW"), tr("FILE") });
	const util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>> mks = break_markers;
	for (auto it = mks.cbegin(); it != mks.cend(); ++it)
	{
		QString fileName = it.key();
		const util::SafeHash<qint64, break_marker_t> mk = mks.value(fileName);
		for (const break_marker_t& bit : mk)
		{
			QTreeWidgetItem* item = q_check_ptr(new QTreeWidgetItem({ bit.content, QString::number(bit.count), QString::number(bit.line + 1), fileName }));
			item->setIcon(0, QIcon("://image/icon_break.png"));
			trees.append(item);
		}
		ui.treeWidget_breakList->addTopLevelItems(trees);
	}

	ui.treeWidget_breakList->setUpdatesEnabled(true);
}

void ScriptSettingForm::onReloadScriptList()
{
	if (IS_LOADING) return;

	TreeWidgetItem* item = nullptr;
	QStringList newScriptList = {};
	do
	{
		item = q_check_ptr(new TreeWidgetItem);
		if (!item) break;

		util::loadAllFileLists(item, util::applicationDirPath() + "/script/", &newScriptList);

		scriptList_ = newScriptList;

		ui.treeWidget_scriptList->setUpdatesEnabled(false);

		ui.treeWidget_scriptList->clear();
		ui.treeWidget_scriptList->addTopLevelItem(item);
		//展開全部第一層
		ui.treeWidget_scriptList->topLevelItem(0)->setExpanded(true);
		for (int i = 0; i < item->childCount(); ++i)
		{
			ui.treeWidget_scriptList->expandItem(item->child(i));
		}

		ui.treeWidget_scriptList->sortItems(0, Qt::AscendingOrder);

		ui.treeWidget_scriptList->setUpdatesEnabled(true);
	} while (false);
}

//全局變量列表
void ScriptSettingForm::onVarInfoImport(QList<QTreeWidgetItem*> nodes)
{
	ui.treeWidget_debuger_custom->setUpdatesEnabled(false);
	ui.treeWidget_debuger_custom->clear();
	ui.treeWidget_debuger_custom->addTopLevelItems(nodes);
	ui.treeWidget_debuger_custom->setUpdatesEnabled(true);

	//if (currentGlobalVarInfo_ == d)
	//	return;

	//currentGlobalVarInfo_ = d;
	//QStringList systemVarList = {

	//};

	//QHash<QString, QVariant> globalVarInfo;
	//QHash<QString, QVariant> customVarInfo;
	////如果key存在於系統變量列表中則添加到系統變量列表中

	//QString key;
	//QStringList l;
	//for (auto it = d.cbegin(); it != d.cend(); ++it)
	//{
	//	l = it.key().split(util::rexOR);
	//	if (l.size() != 2)
	//		continue;

	//	key = l.at(1);
	//	if (systemVarList.contains(key.simplified()))
	//	{
	//		globalVarInfo.insert(it.key(), it.value());
	//	}
	//	else if (!systemVarList.contains(key.simplified()))
	//	{
	//		customVarInfo.insert(it.key(), it.value());
	//	}
	//}

	//varInfoImport(ui.treeWidget_debuger_global, globalVarInfo);
	//varInfoImport(ui.treeWidget_debuger_custom, customVarInfo);
}

void ScriptSettingForm::onCallStackInfoChanged(const QVariant& var)
{
	QVector<QPair<int, QString>> vec = var.value<QVector<QPair<int, QString>>>();
	stackInfoImport(ui.treeWidget_debuger_callstack, vec);
}

void ScriptSettingForm::onJumpStackInfoChanged(const QVariant& var)
{
	QVector<QPair<int, QString>> vec = var.value<QVector<QPair<int, QString>>>();
	stackInfoImport(ui.treeWidget_debuger_jmpstack, vec);
}

void ScriptSettingForm::onEncryptSave()
{
#ifdef CRYPTO_H
	QInputDialog inputDialog(this);
	inputDialog.setWindowTitle(tr("EncryptScript"));
	inputDialog.setLabelText(tr("Please input password"));
	inputDialog.setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);
	inputDialog.setModal(true);
	inputDialog.setInputMode(QInputDialog::TextInput);
	inputDialog.setTextEchoMode(QLineEdit::Password);
	inputDialog.resize(300, 100);
	inputDialog.move(this->pos().x() + this->width() / 2 - inputDialog.width() / 2, this->pos().y() + this->height() / 2 - inputDialog.height() / 2);
	if (inputDialog.exec() != QDialog::Accepted)
		return;

	//隨機生成256位hash 
	QString random = inputDialog.textValue();
	QByteArray hashKey = QCryptographicHash::hash(random.toUtf8(), QCryptographicHash::Sha256);
	//to hax string
	QString password = hashKey.toHex();
	if (password.isEmpty())
	{
		ui.statusBar->showMessage(tr("Encrypt password can not be empty"), 3000);
		return;
	}

	Crypto crypto;
	Injector& injector = Injector::getInstance();
	if (crypto.encodeScript(injector.currentScriptFileName, password))
	{
		QString newFileName = injector.currentScriptFileName;
		newFileName.replace(util::SCRIPT_DEFAULT_SUFFIX, util::SCRIPT_PRIVATE_SUFFIX_DEFAULT);
		injector.currentScriptFileName = newFileName;
		ui.statusBar->showMessage(QString(tr("Encrypt script %1 saved")).arg(newFileName), 3000);
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.loadFileToTable(newFileName);
	}
	else
	{
		ui.statusBar->showMessage(tr("Encrypt script save failed"), 3000);
	}
#endif
}

void ScriptSettingForm::onDecryptSave()
{
#ifdef CRYPTO_H
	QInputDialog inputDialog(this);
	inputDialog.setWindowTitle(tr("DecryptScript"));
	inputDialog.setLabelText(tr("Please input password"));
	inputDialog.setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);
	inputDialog.setModal(true);
	inputDialog.setInputMode(QInputDialog::TextInput);
	inputDialog.setTextEchoMode(QLineEdit::Password);
	inputDialog.resize(300, 100);
	inputDialog.move(this->pos().x() + this->width() / 2 - inputDialog.width() / 2, this->pos().y() + this->height() / 2 - inputDialog.height() / 2);
	if (inputDialog.exec() != QDialog::Accepted)
		return;

	//隨機生成256位hash 
	QString random = inputDialog.textValue();
	QByteArray hashKey = QCryptographicHash::hash(random.toUtf8(), QCryptographicHash::Sha256);
	//to hax string
	QString password = hashKey.toHex();
	if (password.isEmpty())
	{
		ui.statusBar->showMessage(tr("Decrypt password can not be empty"), 3000);
		return;
	}

	Crypto crypto;
	QString content;
	Injector& injector = Injector::getInstance();
	if (crypto.decodeScript(injector.currentScriptFileName, content))
	{
		QString newFileName = injector.currentScriptFileName;
		newFileName.replace(util::SCRIPT_PRIVATE_SUFFIX_DEFAULT, util::SCRIPT_DEFAULT_SUFFIX);
		injector.currentScriptFileName = newFileName;
		ui.statusBar->showMessage(QString(tr("Decrypt script %1 saved")).arg(newFileName), 3000);
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.loadFileToTable(newFileName);
	}
	else
	{
		ui.statusBar->showMessage(tr("Decrypt password is incorrect"), 3000);
	}
#endif
}