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
#include "scripteditor.h"
#include "model/customtitlebar.h"

#include "injector.h"
#include "signaldispatcher.h"

//#include "crypto.h"
#include <QSpinBox>

extern util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>> break_markers[];//interpreter.cpp//用於標記自訂義中斷點(紅點)
extern util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>> forward_markers[];//interpreter.cpp//用於標示當前執行中斷處(黃箭頭)
extern util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>> error_markers[];//interpreter.cpp//用於標示錯誤發生行(紅線)
extern util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>> step_markers[];//interpreter.cpp//隱式標記中斷點用於單步執行(無)

ScriptEditor::ScriptEditor(qint64 index, QWidget* parent)
	: QMainWindow(parent)
	, Indexer(index)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_StyledBackground);

	qRegisterMetaType<QVector<qint64>>();

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

	ui.treeWidget_scriptList->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui.treeWidget_scriptList->resizeColumnToContents(1);
	ui.treeWidget_scriptList->header()->setSectionsClickable(true);
	connect(ui.treeWidget_scriptList->header(), &QHeaderView::sectionClicked, this, &ScriptEditor::onScriptTreeWidgetHeaderClicked, Qt::QueuedConnection);
	connect(ui.treeWidget_scriptList, &QTreeWidget::itemDoubleClicked, this, &ScriptEditor::onScriptTreeWidgetDoubleClicked, Qt::QueuedConnection);
	connect(ui.treeWidget_scriptList, &QTreeWidget::itemChanged, this, &ScriptEditor::onScriptTreeWidgetItemChanged, Qt::QueuedConnection);
	//排序
	ui.treeWidget_functionList->sortItems(0, Qt::AscendingOrder);

	ui.listView_log->setTextElideMode(Qt::ElideNone);
	ui.listView_log->setResizeMode(QListView::Adjust);

	ui.widget->setIndex(index);

	ui.menuBar->setMinimumWidth(100);

	initStaticLabel();

	ui.mainToolBar->show();

	connect(ui.actionSave, &QAction::triggered, this, &ScriptEditor::onActionTriggered, Qt::QueuedConnection);
	connect(ui.actionSaveAs, &QAction::triggered, this, &ScriptEditor::onActionTriggered, Qt::QueuedConnection);
	connect(ui.actionDirectory, &QAction::triggered, this, &ScriptEditor::onActionTriggered, Qt::QueuedConnection);
	connect(ui.actionStep, &QAction::triggered, this, &ScriptEditor::onActionTriggered, Qt::QueuedConnection);
	connect(ui.actionMark, &QAction::triggered, this, &ScriptEditor::onActionTriggered, Qt::QueuedConnection);
	connect(ui.actionNew, &QAction::triggered, this, &ScriptEditor::onActionTriggered, Qt::QueuedConnection);
	connect(ui.actionStart, &QAction::triggered, this, &ScriptEditor::onActionTriggered, Qt::QueuedConnection);
	connect(ui.actionPause, &QAction::triggered, this, &ScriptEditor::onActionTriggered, Qt::QueuedConnection);
	connect(ui.actionStop, &QAction::triggered, this, &ScriptEditor::onActionTriggered, Qt::QueuedConnection);
	connect(ui.actionContinue, &QAction::triggered, this, &ScriptEditor::onActionTriggered, Qt::QueuedConnection);
	connect(ui.actionLogback, &QAction::triggered, this, &ScriptEditor::onActionTriggered, Qt::QueuedConnection);
	connect(ui.actionSaveEncode, &QAction::triggered, this, &ScriptEditor::onActionTriggered, Qt::QueuedConnection);
	connect(ui.actionSaveDecode, &QAction::triggered, this, &ScriptEditor::onActionTriggered, Qt::QueuedConnection);
	connect(ui.actionDebug, &QAction::triggered, this, &ScriptEditor::onActionTriggered, Qt::QueuedConnection);

	connect(this, &ScriptEditor::editorCursorPositionChanged, this, &ScriptEditor::onEditorCursorPositionChanged, Qt::QueuedConnection);
	connect(this, &ScriptEditor::breakMarkInfoImport, this, &ScriptEditor::onBreakMarkInfoImport, Qt::QueuedConnection);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &ScriptEditor::onApplyHashSettingsToUI, Qt::QueuedConnection);

	connect(&signalDispatcher, &SignalDispatcher::scriptStarted, this, &ScriptEditor::onScriptStartMode, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptPaused, this, &ScriptEditor::onScriptPauseMode, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptResumed, this, &ScriptEditor::onScriptStartMode, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptBreaked, this, &ScriptEditor::onScriptBreakMode, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptFinished, this, &ScriptEditor::onScriptStopMode, Qt::QueuedConnection);

	connect(&signalDispatcher, &SignalDispatcher::addForwardMarker, this, &ScriptEditor::onAddForwardMarker, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::addErrorMarker, this, &ScriptEditor::onAddErrorMarker, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::addBreakMarker, this, &ScriptEditor::onAddBreakMarker, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::addStepMarker, this, &ScriptEditor::onAddStepMarker, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::loadFileToTable, this, &ScriptEditor::loadFile, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::reloadScriptList, this, &ScriptEditor::onReloadScriptList, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptLabelRowTextChanged, this, &ScriptEditor::onScriptLabelRowTextChanged, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::varInfoImported, this, &ScriptEditor::onVarInfoImport, Qt::BlockingQueuedConnection);

	//QList <OpenGLWidget*> glWidgetList = util::findWidgets<OpenGLWidget>(this);
	//for (auto& glWidget : glWidgetList)
	//{
	//	if (glWidget)
	//		glWidget->setBackgroundColor(QColor(31, 31, 31));
	//}

	Injector& injector = Injector::getInstance(index);
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

	injector.isScriptEditorOpened.store(true, std::memory_order_release);

	const QString fileName(qgetenv("JSON_PATH"));
	if (fileName.isEmpty())
		return;

	if (!QFile::exists(fileName))
		return;
	util::Config config(fileName);

	QString ObjectName = ui.widget->objectName();
	qint64 fontSize = config.read<qint64>(objectName(), ObjectName, "FontSize");
	if (fontSize > 0)
	{
		QFont font = ui.widget->getOldFont();
		font.setPointSize(fontSize);
		ui.widget->setNewFont(font);
		ui.widget->zoomTo(3);
	}

	ObjectName = ui.listView_log->objectName();
	fontSize = config.read<qint64>(objectName(), ObjectName, "FontSize");
	if (fontSize > 0)
	{
		QFont f = ui.listView_log->font();
		f.setPointSize(fontSize);
		ui.listView_log->setFont(f);
	}

	ObjectName = ui.textBrowser->objectName();
	fontSize = config.read<qint64>(objectName(), ObjectName, "FontSize");
	if (fontSize > 0)
	{
		QFont font = ui.textBrowser->font();
		font.setPointSize(fontSize);
		ui.textBrowser->setFont(font);
	}
}

void ScriptEditor::initStaticLabel()
{
	lineLable_ = new FastLabel(tr("row:%1").arg(1), QColor("#FFFFFF"), QColor(64, 53, 130), ui.statusBar);
	Q_ASSERT(lineLable_ != nullptr);
	lineLable_->setFixedWidth(60);
	lineLable_->setTextColor(QColor(255, 255, 255));
	sizeLabel_ = new FastLabel("| " + tr("size:%1").arg(0), QColor("#FFFFFF"), QColor(64, 53, 130), ui.statusBar);
	Q_ASSERT(sizeLabel_ != nullptr);
	sizeLabel_->setFixedWidth(60);
	sizeLabel_->setTextColor(QColor(255, 255, 255));
	indexLabel_ = new FastLabel("| " + tr("index:%1").arg(1), QColor("#FFFFFF"), QColor(64, 53, 130), ui.statusBar);
	Q_ASSERT(indexLabel_ != nullptr);
	indexLabel_->setFixedWidth(60);
	indexLabel_->setTextColor(QColor(255, 255, 255));

	const QsciScintilla::EolMode mode = ui.widget->eolMode();
	const QString modeStr(mode == QsciScintilla::EolWindows ? "CRLF" : mode == QsciScintilla::EolUnix ? "  LF" : "  CR");
	eolLabel_ = new FastLabel(QString("| %1").arg(modeStr), QColor("#FFFFFF"), QColor(64, 53, 130), ui.statusBar);
	Q_ASSERT(eolLabel_ != nullptr);
	eolLabel_->setFixedWidth(50);
	eolLabel_->setTextColor(QColor(255, 255, 255));

	usageLabel_ = new FastLabel(QString(tr("Usage: cpu: %1% | memory: %2MB / %3MB"))
		.arg(0).arg(0).arg(0), QColor("#FFFFFF"), QColor(64, 53, 130), ui.statusBar);
	Q_ASSERT(usageLabel_ != nullptr);
	usageLabel_->setFixedWidth(350);
	usageLabel_->setTextColor(QColor(255, 255, 255));

	QLabel* spaceLabeRight = new QLabel("", ui.statusBar);
	Q_ASSERT(spaceLabeRight != nullptr);
	spaceLabeRight->setFrameStyle(QFrame::NoFrame);
	spaceLabeRight->setFixedWidth(10);

	QLabel* spaceLabelMiddle = new QLabel("", ui.statusBar);
	Q_ASSERT(spaceLabelMiddle != nullptr);
	spaceLabelMiddle->setFrameStyle(QFrame::NoFrame);
	spaceLabelMiddle->setFixedWidth(100);

	usageTimer_ = new QTimer(this);
	Q_ASSERT(usageTimer_ != nullptr);
	usageTimer_->setInterval(1500);
	connect(usageTimer_, &QTimer::timeout, this, [this]()
		{
			qint64 currentIndex = getIndex();
			Injector& injector = Injector::getInstance(currentIndex);
			quint64 threadId = reinterpret_cast<quint64>(QThread::currentThreadId());
			if (injector.IS_SCRIPT_FLAG.load(std::memory_order_acquire))
			{
				threadId = injector.scriptThreadId;
			}

			qreal cpuUsage = 0;
			qreal memoryUsage = 0;
			qreal memoryTotal = 0;

			if (util::monitorThreadResourceUsage(threadId, idleTime_, kernelTime_, userTime_, &cpuUsage, &memoryUsage, &memoryTotal))
			{
				usageLabel_->setText(QString(tr("Usage: cpu: %1% | memory: %2MB / %3MB"))
					.arg(util::toQString(cpuUsage))
					.arg(util::toQString(memoryUsage))
					.arg(util::toQString(memoryTotal)));
			}
		});

	usageTimer_->start();

	ui.statusBar->setAttribute(Qt::WA_StyledBackground);

	ui.statusBar->setStyleSheet("color: rgb(255, 255, 255); background-color: rgb(64, 53, 130); border:none");

	ui.statusBar->addPermanentWidget(usageLabel_);

	ui.statusBar->addPermanentWidget(spaceLabelMiddle);

	ui.statusBar->addPermanentWidget(lineLable_);

	ui.statusBar->addPermanentWidget(sizeLabel_);

	ui.statusBar->addPermanentWidget(indexLabel_);

	ui.statusBar->addPermanentWidget(eolLabel_);

	ui.statusBar->addPermanentWidget(spaceLabeRight);
}

void ScriptEditor::createSpeedSpinBox()
{
	pSpeedDescLabel_ = new QLabel(tr("Script speed:"), ui.mainToolBar);
	Q_ASSERT(pSpeedDescLabel_ != nullptr);
	pSpeedDescLabel_->setAttribute(Qt::WA_StyledBackground);

	pSpeedSpinBox = new QSpinBox(ui.mainToolBar);
	Q_ASSERT(pSpeedSpinBox != nullptr);
	pSpeedSpinBox->setStyleSheet("QLabel {color: #FAFAFA; font-size: 12pt;}");

	qint64 currentIndex = getIndex();
	pSpeedSpinBox->setRange(0, 10000);
	Injector& injector = Injector::getInstance(currentIndex);
	qint64 value = injector.getValueHash(util::kScriptSpeedValue);
	pSpeedSpinBox->setValue(value);
	pSpeedSpinBox->setAttribute(Qt::WA_StyledBackground);
	pSpeedSpinBox->setSingleStep(100);
	pSpeedSpinBox->setStyleSheet(R"(
		QSpinBox {
			border:1px solid #3D3D3D;
 			color: #FAFAFA;
  			background: #333333;
			selection-color: #FFFFFF;
			selection-background-color: #0078D7;
			font-size: 12pt;
		}

		QSpinBox:hover {
		  color: #FAFAFA;
		  background: rgb(61,61,61);
		  border:1px solid #424242;
		}

		QSpinBox::up-arrow {
			image: url(:/image/icon_glyphup.svg);
			width: 14px;
			height: 14px;
		}

		QSpinBox::down-arrow {
			image: url(:/image/icon_glyphdown.svg);
			width: 14px;
			height: 14px;
		}

		QSpinBox::down-button, QSpinBox::up-button {
			background: #333333;
		}


		QSpinBox::up-button:hover, QSpinBox::down-button:hover{
			background: rgb(61,61,61);
		}

		QSpinBox::up-button:pressed, QSpinBox::down-button:pressed {
			background: rgb(91,91,91);
		}


	)");

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	connect(&signalDispatcher, &SignalDispatcher::scriptSpeedChanged, this, &ScriptEditor::onSpeedChanged);

	connect(pSpeedSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ScriptEditor::onSpeedChanged);

	ui.mainToolBar->addWidget(pSpeedDescLabel_);
	ui.mainToolBar->addWidget(pSpeedSpinBox);
}

ScriptEditor::~ScriptEditor()
{
	qDebug() << "~ScriptEditor";
	Injector& injector = Injector::getInstance(getIndex());
	injector.isScriptEditorOpened.store(false, std::memory_order_release);
}

void ScriptEditor::showEvent(QShowEvent* e)
{
	setAttribute(Qt::WA_Mapped);
	QMainWindow::showEvent(e);
}

void ScriptEditor::closeEvent(QCloseEvent* e)
{
	do
	{
		Injector& injector = Injector::getInstance(getIndex());
		injector.isScriptEditorOpened.store(false, std::memory_order_release);

		if (usageTimer_ != nullptr)
			usageTimer_->stop();

		util::FormSettingManager formSettingManager(this);
		formSettingManager.saveSettings();

		const QString fileName(qgetenv("JSON_PATH"));
		if (fileName.isEmpty())
			break;

		if (!QFile::exists(fileName))
			break;

		util::Config config(fileName);

		if (!injector.currentScriptFileName.isEmpty())
			config.write("Script", "LastModifyFile", injector.currentScriptFileName);


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

//////////////////////

void ScriptEditor::replaceCommas(QString& input)
{
	input = input.simplified();
	if (input.startsWith("//"))
	{
		if (!input.startsWith("// "))
		{
			input.insert(2, " ");
		}
		return;
	}

	static const QRegularExpression regexComma(R"(\s*(,)\s*(?=(?:[^'"]*(['"])(?:[^'"]*\\.)*[^'"]*\2)*[^'"]*$))");
	input.replace(regexComma, ", ");

	static const QRegularExpression regexLeft(R"(\s*(\{)\s*(?=(?:[^'"]*(['"])(?:[^'"]*\\.)*[^'"]*\2)*[^'"]*$))");
	input.replace(regexLeft, " { ");

	static const QRegularExpression regexRight(R"((?=(?:[^'"]*(['"])(?:[^'"]*\\.)*[^'"]*\2)*[^'"]*)\s*(\})\s*$)");
	input.replace(regexRight, " }");

	QStringList opList = {
		"+", "-", "*",  "%", "&", "|", "^", ">", "<", "=", "!",
	};

	for (const QString it : opList)
	{
		const QRegularExpression regex(QString(R"(\s*(\%1\=)\s*(?=(?:[^'"]*(['"])(?:[^'"]*\\.)*[^'"]*\2)*[^'"]*$))").arg(it));
		auto match = regex.match(input);
		if (match.hasMatch())
		{
			input.replace(regex, QString(" %1 ").arg(match.captured(1).simplified()));
		}
	}

	//for (const QString& op : opList)
	//{
	//	const QRegularExpression regex(QString(R"(\s*%1\s*)").arg(QRegularExpression::escape(op)));
	//	input.replace(regex, QString(" %1 ").arg(op));
	//}

	input = input.simplified();
	input.replace("= =", "==");
	input.replace("! =", "!=");
	input.replace("> =", ">=");
	input.replace("< =", "<=");
	input.replace("= >", "=>");
	input.replace("= <", "=<");
	input.replace("+ =", "+=");
	input.replace("- =", "-=");
	input.replace("* =", "*=");
	input.replace("/ =", "/=");
	input.replace("% =", "%=");
	input.replace("& =", "&=");
	input.replace("| =", "|=");
	input.replace("^ =", "^=");

	static const QRegularExpression regexDivEq(R"((?=(?:[^'"]*(['"])(?:[^'"]*\\.)*[^'"]*\2)*[^'"]*)\s*(\/\=)\s*(?=(?:[^'"]*(['"])(?:[^'"]*\\.)*[^'"]*\2)*[^'"]*))");
	input.replace(regexDivEq, " /= ");

	const QRegularExpression regexadd(R"(([\p{Han}|\w|,]\s+)\+\s+(\d+)$)");
	auto match = regexadd.match(input);
	if (match.hasMatch())
	{
		input.replace(regexadd, "\\1+\\2");
	}

	const QRegularExpression regexadd2(R"(([,|> |< |\= ]\s+)\+\s+(\d+)([,\s+]))");
	match = regexadd2.match(input);
	if (match.hasMatch())
	{
		input.replace(regexadd2, "\\1+\\2\\3");
	}

	const QRegularExpression regexsub(R"(([\p{Han}|\w|,]\s+)\-\s+(\d+)$)");
	match = regexsub.match(input);
	if (match.hasMatch())
	{
		input.replace(regexsub, "\\1-\\2");
	}

	const QRegularExpression regexsub2(R"(([,|> |< |\= ]\s+)\-\s+(\d+)([,\s+]))");
	match = regexsub2.match(input);
	if (match.hasMatch())
	{
		input.replace(regexsub2, "\\1-\\2\\3");
	}

	input.replace(" + +", "++");
	input.replace(" - -", "--");

	const QRegularExpression regexFunc(R"(function\s+([\p{Han}\w]+)\s+\()");
	match = regexFunc.match(input);
	if (match.hasMatch())
	{
		input.replace(regexFunc, "function \\1(");
	}

	const QRegularExpression regexCallFunc(R"((^\s*[\p{Han}\w]+)\s+\()");
	match = regexCallFunc.match(input);
	if (match.hasMatch())
	{
		input.replace(regexCallFunc, "\\1(");
	}

	if (!input.contains("for"))
		input.replace(", ,", ", '',");

	input.replace("< <=", "<<=");
	input.replace("> >=", ">>=");
	input.replace("{ }", "{}");
}

QString ScriptEditor::formatCode(QString content)
{
	content.replace("\r\n", "\n");
	QStringList contents = content.split("\n");
	QStringList newContents;
	qint64 indentLevel = 0;
	QString indentedLine;
	QString tmpLine;

	bool beginLuaCode = false;

	auto checkLua = [&beginLuaCode](QString& line)->bool
	{
		if (line.trimmed().toLower().startsWith("#lua") && !beginLuaCode)
		{
			beginLuaCode = true;
			line = line.trimmed().toLower();
			return true;
		}
		else if (line.trimmed().toLower().startsWith("#endlua") && beginLuaCode)
		{
			beginLuaCode = false;
			line = line.trimmed().toLower();
			return true;
		}
		else if (beginLuaCode)
		{
			return true;
		}

		return false;
	};

	for (QString& line : contents)
	{
		if (line.isEmpty())
			continue;

		if (checkLua(line))
			continue;

		replaceCommas(line);
	}

	beginLuaCode = false;

	QString raw = "";
	QString trimmedLine = "";
	qint64 endIndex = -1;
	qint64 commandIndex = -1;
	for (const QString& line : contents)
	{
		raw = line;
		if (checkLua(raw))
		{
			newContents.append(raw);
			continue;
		}

		trimmedLine = raw.trimmed();
		endIndex = trimmedLine.indexOf("end");
		if (-1 == endIndex)
		{
			endIndex = trimmedLine.indexOf("until");
		}

		tmpLine.clear();
		if (endIndex != -1)
		{
			commandIndex = trimmedLine.simplified().indexOf("//");
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
		else if (trimmedLine.startsWith("for ") || trimmedLine.startsWith("for (") || trimmedLine.startsWith("for(") || trimmedLine == "for")
		{
			indentLevel++;
		}
		else if (trimmedLine.startsWith("while ") || trimmedLine.startsWith("while (") || trimmedLine.startsWith("while(") || trimmedLine == "while")
		{
			indentLevel++;
		}
		else if (trimmedLine == "repeat")
		{
			indentLevel++;
		}
	}

	return newContents.join("\n");
}

void ScriptEditor::fileSave(QString content)
{
	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
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

	content = formatCode(content);

	if (!util::writeFile(fileName, content))
		return;

	ui.statusBar->showMessage(QString(tr("Script %1 saved")).arg(fileName), 10000);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.addForwardMarker(-1, false);
	emit signalDispatcher.addErrorMarker(-1, false);
	emit signalDispatcher.addStepMarker(-1, false);

	forward_markers[currentIndex].clear();
	error_markers[currentIndex].clear();
	step_markers[currentIndex].clear();
	reshowBreakMarker();

	emit ui.comboBox_labels->clicked();
	emit ui.comboBox_functions->clicked();
	emit ui.widget->modificationChanged(true);
}

void ScriptEditor::loadFile(const QString& fileName)
{
	QString content;
	if (!util::readFile(fileName, &content))
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
	qint64 scollValue = ui.widget->verticalScrollBar()->value();

	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);

	if (!injector.server.isNull() && injector.server->getOnlineFlag())
		setWindowTitle(QString("[%1][%2] %3").arg(currentIndex).arg(injector.server->getPC().name).arg(fileName));
	else
		setWindowTitle(QString("[%1] %2").arg(currentIndex).arg(fileName));

	ui.widget->setUpdatesEnabled(false);

	ui.widget->setModified(false);
	ui.widget->selectAll();
	ui.widget->replaceSelectedText(content);

	ui.widget->setUpdatesEnabled(true);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.addForwardMarker(-1, false);
	emit signalDispatcher.addErrorMarker(-1, false);
	emit signalDispatcher.addStepMarker(-1, false);

	forward_markers[currentIndex].clear();
	error_markers[currentIndex].clear();
	step_markers[currentIndex].clear();

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

	if (injector.IS_SCRIPT_FLAG.load(std::memory_order_acquire))
	{
		onScriptStartMode();
	}
}

void ScriptEditor::setContinue()
{
	qint64 currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);

	emit signalDispatcher.addForwardMarker(-1, false);
	emit signalDispatcher.addErrorMarker(-1, false);
	emit signalDispatcher.addStepMarker(-1, false);

	forward_markers[currentIndex].clear();
	error_markers[currentIndex].clear();
	step_markers[currentIndex].clear();

	emit signalDispatcher.scriptResumed();
}

QString ScriptEditor::getFullPath(TreeWidgetItem* item)
{
	QStringList filepath;
	TreeWidgetItem* itemfile = item; //獲取被點擊的item
	while (itemfile != NULL)
	{
		filepath << itemfile->text(0); //獲取itemfile名稱
		itemfile = reinterpret_cast<TreeWidgetItem*>(itemfile->parent()); //將itemfile指向父item
	}
	QString strpath;
	qint64 count = (static_cast<qint64>(filepath.size()) - 1);
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

	QFileInfo info(strpath);
	QString suffix = "." + info.suffix();
	if (suffix.isEmpty())
		strpath += util::SCRIPT_DEFAULT_SUFFIX;
	if (suffix != util::SCRIPT_PRIVATE_SUFFIX_DEFAULT && suffix != util::SCRIPT_DEFAULT_SUFFIX && suffix != util::SCRIPT_LUA_SUFFIX_DEFAULT)
		strpath.replace(suffix, util::SCRIPT_DEFAULT_SUFFIX);

	return strpath;
};

void ScriptEditor::createScriptListContextMenu()
{
	// Create the custom context menu
	QMenu* menu = new QMenu(this);
	Q_ASSERT(menu != nullptr);
	if (menu == nullptr)
		return;

	QAction* openAcrion = menu->addAction(tr("open"));
	QAction* deleteAction = menu->addAction(tr("delete"));
	QAction* renameAction = menu->addAction(tr("rename"));

	connect(openAcrion, &QAction::triggered, this, [this]()
		{
			//current item
			TreeWidgetItem* item = reinterpret_cast<TreeWidgetItem*>(ui.treeWidget_scriptList->currentItem());
			if (!item)
				return;

			onScriptTreeWidgetDoubleClicked(item, 0);
		});

	qint64 currentIndex = getIndex();

	// Connect the menu actions to your slots/functions
	connect(deleteAction, &QAction::triggered, this, [this, currentIndex]()
		{
			//current item
			TreeWidgetItem* item = reinterpret_cast<TreeWidgetItem*>(ui.treeWidget_scriptList->currentItem());
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

			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
			emit signalDispatcher.reloadScriptList();
		});

	connect(renameAction, &QAction::triggered, this, [this]()
		{
			//current item
			TreeWidgetItem* item = reinterpret_cast<TreeWidgetItem*>(ui.treeWidget_scriptList->currentItem());
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
	connect(ui.treeWidget_scriptList, &QTreeWidget::customContextMenuRequested, this, [this, menu, currentIndex](const QPoint& pos)
		{
			TreeWidgetItem* item = reinterpret_cast<TreeWidgetItem*>(ui.treeWidget_scriptList->itemAt(pos));
			if (item)
			{
				// Show the context menu at the requested position
				menu->exec(ui.treeWidget_scriptList->mapToGlobal(pos));

				SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
				emit signalDispatcher.reloadScriptList();
			}
		});
}

void ScriptEditor::setMark(CodeEditor::SymbolHandler element, util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>>& hash, qint64 liner, bool b)
{
	do
	{
		if (liner == -1)
		{
			hash.clear();
			ui.widget->markerDeleteAll(element);
			break;
		}

		Injector& injector = Injector::getInstance(getIndex());

		QString currentScriptFileName = injector.currentScriptFileName;
		if (currentScriptFileName.isEmpty())
			break;


		util::SafeHash<qint64, break_marker_t> markers = hash.value(currentScriptFileName);

		if (b)
		{
			break_marker_t bk = markers.value(liner);
			bk.line = liner;
			bk.content = ui.widget->text(liner);
			bk.maker = static_cast<qint64>(element);
			markers.insert(liner, bk);
			hash.insert(currentScriptFileName, markers);
			ui.widget->markerAdd(liner, element);
			emit editorCursorPositionChanged(liner, 0);
		}
		else
		{
			if (markers.contains(liner))
			{
				markers.remove(liner);
				hash.insert(currentScriptFileName, markers);
				ui.widget->markerDelete(liner, element);
			}
		}
	} while (false);
}

void ScriptEditor::setStepMarks()
{
	QString content = ui.widget->text();
	QStringList list = content.split("\n");
	if (list.isEmpty())
		return;

	qint64 maxliner = list.size();
	qint64 index = 1;

	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);


	QString currentScriptFileName = injector.currentScriptFileName;
	if (currentScriptFileName.isEmpty())
		return;

	util::SafeHash<qint64, break_marker_t> markers = step_markers[currentIndex].value(currentScriptFileName);
	break_marker_t bk = {};

	for (qint64 i = 0; i < maxliner; ++i)
	{
		index = i + 1;
		bk = markers.value(index);
		bk.line = index;
		bk.content = list.value(i);
		bk.maker = static_cast<qint64>(CodeEditor::SymbolHandler::SYM_STEP);
		markers.insert(index, bk);
	}

	step_markers[currentIndex].insert(currentScriptFileName, markers);
}

void ScriptEditor::reshowBreakMarker()
{
	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	const util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>> mks = break_markers[currentIndex];

	QString currentScriptFileName = injector.currentScriptFileName;
	if (currentScriptFileName.isEmpty())
		return;

	for (auto it = mks.cbegin(); it != mks.cend(); ++it)
	{
		QString fileName = it.key();
		if (fileName != currentScriptFileName)
			continue;

		const util::SafeHash<qint64, break_marker_t> mk = mks.value(fileName);
		for (const break_marker_t& bit : mk)
		{
			ui.widget->markerAdd(bit.line, CodeEditor::SymbolHandler::SYM_POINT);
		}
	}

	emit breakMarkInfoImport();
}
//////////////////////

void ScriptEditor::on_widget_marginClicked(int margin, int line, Qt::KeyboardModifiers state)
{
	Q_UNUSED(margin);
	qint64 mask = ui.widget->markersAtLine(line);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
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

void ScriptEditor::onSetStaticLabelLineText(int line, int index)
{
	lineLable_->setText(tr("row:%1").arg(line + 1));
	sizeLabel_->setText("| " + tr("size:%1").arg(ui.widget->text(line).size()));
	indexLabel_->setText("| " + tr("index:%1").arg(index));
}

void ScriptEditor::on_widget_cursorPositionChanged(int line, int index)
{
	Injector& injector = Injector::getInstance(getIndex());
	if (!injector.IS_SCRIPT_FLAG.load(std::memory_order_acquire))
		onSetStaticLabelLineText(line, index);
}

void ScriptEditor::on_widget_textChanged()
{
	Injector& injector = Injector::getInstance(getIndex());
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

void ScriptEditor::on_comboBox_labels_clicked()
{
	static const QRegularExpression rex_divLabel(R"((label\s+[\p{Han}\w]+))");
	ui.comboBox_labels->blockSignals(true);

	//將所有lua function 和 label加入 combobox並使其能定位行號
	int line = -1, index = -1;
	ui.widget->getCursorPosition(&line, &index);

	const QString contents = ui.widget->text();
	const QStringList conlist = contents.split("\n");
	qint64 cur = ui.comboBox_labels->currentIndex();
	ui.comboBox_labels->clear();

	qint64 count = conlist.size();
	for (qint64 i = 0; i < count; ++i)
	{
		const QString linestr = conlist.value(i).simplified();
		if (linestr.isEmpty())
			continue;
		QRegularExpressionMatchIterator it = rex_divLabel.globalMatch(linestr);
		while (it.hasNext())
		{
			const QRegularExpressionMatch match = it.next();
			const QString str = match.captured(1).simplified();
			if (!str.isEmpty())
			{
				ui.comboBox_labels->addItem(QIcon(":/image/icon_smarttag.svg"), str.simplified(), QVariant(i));
			}
		}
	}

	if (cur < 0 || cur >= ui.comboBox_functions->count())
		cur = 0;

	ui.comboBox_labels->setCurrentIndex(cur);

	ui.comboBox_labels->blockSignals(false);
}

//標記列表點擊
void ScriptEditor::on_comboBox_labels_currentIndexChanged(int)
{
	QVariant var = ui.comboBox_labels->currentData();
	if (var.isValid())
	{
		qint64 line = var.toLongLong();
		emit editorCursorPositionChanged(line, NULL);
	}
}

void ScriptEditor::on_comboBox_functions_clicked()
{
	static const QRegularExpression rex_divLabel(R"((function\s+[\p{Han}\w]+))");
	ui.comboBox_functions->blockSignals(true);
	//將所有lua function 和 label加入 combobox並使其能定位行號
	int line = -1, index = -1;
	ui.widget->getCursorPosition(&line, &index);

	const QString contents = ui.widget->text();
	const QStringList conlist = contents.split("\n");
	qint64 cur = ui.comboBox_functions->currentIndex();
	ui.comboBox_functions->clear();

	qint64 count = conlist.size();
	for (qint64 i = 0; i < count; ++i)
	{
		const QString linestr = conlist.value(i).simplified();
		if (linestr.isEmpty())
			continue;
		QRegularExpressionMatchIterator it = rex_divLabel.globalMatch(linestr);
		while (it.hasNext())
		{
			const QRegularExpressionMatch match = it.next();
			const QString str = match.captured(1).simplified();
			if (!str.isEmpty())
			{
				ui.comboBox_functions->addItem(QIcon(":/image/icon_method.svg"), str.simplified(), QVariant(i));
			}
		}
	}

	if (cur < 0 || cur >= ui.comboBox_functions->count())
		cur = 0;

	ui.comboBox_functions->setCurrentIndex(cur);
	ui.comboBox_functions->blockSignals(false);
}

void ScriptEditor::on_comboBox_functions_currentIndexChanged(int)
{
	QVariant var = ui.comboBox_functions->currentData();
	if (var.isValid())
	{
		qint64 line = var.toLongLong();
		emit editorCursorPositionChanged(line, NULL);
	}
}

void ScriptEditor::on_treeWidget_functionList_itemDoubleClicked(QTreeWidgetItem* item, int column)
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

	Injector& injector = Injector::getInstance(getIndex());

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
		qint64 dir = (injector.server->getPC().dir + 3) % 8;
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
		qint64 dir = (injector.server->getPC().dir + 3) % 8;
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
		qint64 floor = injector.server->getFloor();
		str = QString("%1 %2, +2").arg(str).arg(floor);
	}
	else if (str == "waitmap")
	{
		qint64 floor = injector.server->getFloor();
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

void ScriptEditor::on_treeWidget_functionList_itemClicked(QTreeWidgetItem*, int)
{
	emit ui.treeWidget_functionList->itemSelectionChanged();
}

void ScriptEditor::on_treeWidget_functionList_itemSelectionChanged()
{
	do
	{
		TreeWidgetItem* item = reinterpret_cast<TreeWidgetItem*>(ui.treeWidget_functionList->currentItem());
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

		QSharedPointer<QTextDocument> doc = QSharedPointer<QTextDocument>(new QTextDocument());
		if (doc.isNull())
			break;

		doc->setMarkdown(markdownText);
		document_.insert(str, doc);

		ui.textBrowser->setUpdatesEnabled(false);
		ui.textBrowser->setDocument(doc.data());
		ui.textBrowser->setUpdatesEnabled(true);
	} while (false);
}

void ScriptEditor::on_treeWidget_scriptList_itemClicked(QTreeWidgetItem* item, int column)
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
	constexpr qint64 padding = 20; // 可以根據需要調整
	textRect.adjust(-padding, -padding, padding, padding);

	// 檢查滑鼠位置是否在文字範圍內
	QPoint pos = ui.treeWidget_scriptList->mapFromGlobal(QCursor::pos());
	if (!textRect.contains(pos))
		return;

	ui.treeWidget_scriptList->setCurrentItem(item);


	QString currentText = item->text(0);
	if (currentText.isEmpty())
		return;

	QString currentPath = getFullPath(reinterpret_cast<TreeWidgetItem*>(item));
	if (!QFile::exists(currentPath))
		return;

	//設置為可編輯
	//item->setFlags(item->flags() | Qt::ItemIsEditable);

	currentRenamePath_ = currentPath;
	currentRenameText_ = currentText;

	ui.treeWidget_scriptList->editItem(item, 0);
}

void ScriptEditor::on_treeWidget_breakList_itemDoubleClicked(QTreeWidgetItem* item, int)
{
	if (item == nullptr)
		return;
	Injector& injector = Injector::getInstance(getIndex());
	if (item->text(2).isEmpty())
		return;

	if (injector.IS_SCRIPT_FLAG)
		return;

	if (item->text(3).isEmpty())
		return;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
	injector.currentScriptFileName = item->text(3);
	emit signalDispatcher.loadFileToTable(item->text(3));
	qint64 line = item->text(2).toLongLong();
	QString text = ui.widget->text(line - 1);
	ui.widget->setSelection(line - 1, 0, line - 1, text.length());
	ui.widget->ensureLineVisible(line - 1);
}

//查找命令
void ScriptEditor::on_lineEdit_searchFunction_textChanged(const QString& text)
{
	auto OnFindItem = [](QTreeWidget* tree, const QString& qsFilter)->void
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
				TreeWidgetItem* item = reinterpret_cast<TreeWidgetItem*>(*it);
				//顯示父節點
				while (item->parent())
				{
					item->parent()->setHidden(false);
					item = reinterpret_cast<TreeWidgetItem*>(item->parent());
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

void ScriptEditor::on_listView_log_doubleClicked(const QModelIndex& index)
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
			//ui.widget->setCursorPosition(line.toLongLong(), 0);
			text = ui.widget->text(line.toLongLong() - 1);
			ui.widget->setSelection(line.toLongLong() - 1, 0, line.toLongLong() - 1, text.length());
			ui.widget->ensureLineVisible(line.toLongLong() - 1);
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
			//ui.widget->setCursorPosition(line.toLongLong(), 0);
			text = ui.widget->text(line.toLongLong() - 1);
			ui.widget->setSelection(line.toLongLong() - 1, 0, line.toLongLong() - 1, text.length());
			ui.widget->ensureLineVisible(line.toLongLong() - 1);
			return;
		}
	}
}

void ScriptEditor::onApplyHashSettingsToUI()
{
	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (!injector.server.isNull() && injector.server->getOnlineFlag())
	{
		QString title = injector.currentScriptFileName;
		QString newTitle = QString("[%1][%2] %3").arg(currentIndex).arg(injector.server->getPC().name).arg(title);
		setWindowTitle(newTitle);
	}

	if (!injector.scriptLogModel.isNull())
		ui.listView_log->setModel(injector.scriptLogModel.get());

	pSpeedSpinBox->setValue(injector.getValueHash(util::kScriptSpeedValue));

	ui.actionDebug->setChecked(injector.isScriptDebugModeEnable.load(std::memory_order_acquire));
}

void ScriptEditor::onWidgetModificationChanged(bool changed)
{
	if (!changed) return;
	Injector& injector = Injector::getInstance(getIndex());
	scripts_.insert(injector.currentScriptFileName, ui.widget->text());
}

void ScriptEditor::onScriptTreeWidgetDoubleClicked(QTreeWidgetItem* item, int column)
{
	if (!item)
		return;

	Q_UNUSED(column);
	if (item->text(0).isEmpty())
		return;

	IS_LOADING = true;

	do
	{
		qint64 currnetIndex = getIndex();
		Injector& injector = Injector::getInstance(currnetIndex);
		if (injector.IS_SCRIPT_FLAG.load(std::memory_order_acquire))
			break;

		/*得到文件路徑*/
		QStringList filepath;
		TreeWidgetItem* itemfile = reinterpret_cast<TreeWidgetItem*>(item); //獲取被點擊的item
		while (itemfile != NULL)
		{
			filepath << itemfile->text(0); //獲取itemfile名稱
			itemfile = reinterpret_cast<TreeWidgetItem*>(itemfile->parent()); //將itemfile指向父item
		}
		QString strpath;
		qint64 count = (static_cast<qint64>(filepath.size()) - 1);
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

		QFileInfo fileinfo(strpath);
		if (!fileinfo.isFile())
			break;

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currnetIndex);
		emit signalDispatcher.addForwardMarker(-1, false);
		emit signalDispatcher.addErrorMarker(-1, false);
		emit signalDispatcher.addStepMarker(-1, false);

		forward_markers[currnetIndex].clear();
		error_markers[currnetIndex].clear();
		step_markers[currnetIndex].clear();

		if (!injector.IS_SCRIPT_FLAG.load(std::memory_order_acquire))
			injector.currentScriptFileName = strpath;

		emit signalDispatcher.loadFileToTable(strpath);

	} while (false);

	IS_LOADING = false;
}

void ScriptEditor::onScriptTreeWidgetHeaderClicked(int logicalIndex)
{
	if (logicalIndex == 0)
	{
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
		emit signalDispatcher.reloadScriptList();
	}
}

void ScriptEditor::onScriptTreeWidgetItemChanged(QTreeWidgetItem* newitem, int column)
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

	QString str = getFullPath(reinterpret_cast<TreeWidgetItem*>(newitem));
	if (str.isEmpty())
		return;

	if (str == currentRenamePath_)
		return;

	QFile file(currentRenamePath_);
	if (file.exists() && !QFile::exists(str))
	{
		file.rename(str);
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
	emit signalDispatcher.reloadScriptList();
}

void ScriptEditor::onScriptLabelRowTextChanged(int line, int, bool)
{
	Injector& injector = Injector::getInstance(getIndex());
	if (line < 0)
		line = 1;

	if (!injector.isScriptDebugModeEnable.load(std::memory_order_acquire))
	{
		onSetStaticLabelLineText(line - 1, NULL);
		return;
	}

	ui.widget->setUpdatesEnabled(false);
	QString text = ui.widget->text(line - 1);
	ui.widget->setSelection(line - 1, 0, line - 1, text.size());
	ui.widget->setCaretLineBackgroundColor(QColor(71, 71, 71));//光標所在行背景顏色
	ui.widget->setUpdatesEnabled(true);

	//確保光標示在可視範圍內
	ui.widget->ensureLineVisible(line - 1);

	int index = 0;
	ui.widget->getCursorPosition(&line, &index);
	onSetStaticLabelLineText(line, NULL);
}

//切換光標和焦點所在行
void ScriptEditor::onEditorCursorPositionChanged(int line, int index)
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

void ScriptEditor::onSpeedChanged(int value)
{
	pSpeedSpinBox->blockSignals(true);
	pSpeedSpinBox->setValue(value);
	pSpeedSpinBox->blockSignals(false);

	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	injector.setValueHash(util::kScriptSpeedValue, value);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.applyHashSettingsToUI();
}

void ScriptEditor::onNewFile()
{
	qint64 currnetIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currnetIndex);
	qint64 num = 1;
	for (;;)
	{
		QString strpath = (util::applicationDirPath() + QString("/script/Untitled-%1.txt").arg(num));
		if (!QFile::exists(strpath))
		{
			if (util::writeFile(strpath, ""))
				loadFile(strpath);

			emit signalDispatcher.reloadScriptList();
			break;
		}

		++num;
	}
}

void ScriptEditor::onActionTriggered()
{
	QAction* pAction = qobject_cast<QAction*>(sender());
	if (!pAction)
		return;
	qint64 currnetIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currnetIndex);
	QString name = pAction->objectName();
	if (name.isEmpty())
		return;

	Injector& injector = Injector::getInstance(currnetIndex);

	if (name == "actionSave")
	{
		if (!injector.scriptLogModel.isNull())
			injector.scriptLogModel->clear();

		fileSave(ui.widget->text());
		emit signalDispatcher.loadFileToTable(injector.currentScriptFileName);
		return;
	}

	if (name == "actionStart")
	{
		if (step_markers[currnetIndex].size() == 0 && !injector.IS_SCRIPT_FLAG.load(std::memory_order_acquire))
		{
			emit signalDispatcher.scriptStarted();
		}
		return;
	}

	if (name == "actionPause")
	{
		emit signalDispatcher.scriptPaused();
		return;
	}

	if (name == "actionStop")
	{
		emit signalDispatcher.scriptStoped();
		return;
	}

	if (name == "actionContinue")
	{
		setContinue();
		return;
	}

	if (name == "actionStep")
	{
		emit signalDispatcher.addForwardMarker(-1, false);
		emit signalDispatcher.addErrorMarker(-1, false);
		emit signalDispatcher.scriptBreaked();
		return;
	}

	if (name == "actionSaveAs")
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
			const QString f(fd.selectedFiles().value(0));
			if (f.isEmpty())
				return;

			if (util::writeFile(f, ui.widget->text()))
			{
				QDesktopServices::openUrl(QUrl::fromLocalFile(directoryName));
			}
		}
		emit signalDispatcher.reloadScriptList();
		return;
	}

	if (name == "actionDirectory")
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(util::applicationDirPath() + "/script"));
	}

	if (name == "actionMark")
	{
		int line = -1;
		int index = -1;
		ui.widget->getCursorPosition(&line, &index);
		emit ui.widget->marginClicked(NULL, line, Qt::NoModifier);
		return;
	}

	if (name == "actionLogback")
	{
		if (injector.server.isNull())
			return;

		if (injector.isValid())
			injector.setEnableHash(util::kLogBackEnable, true);
		return;
	}

	if (name == "actionNew")
	{
		QMetaObject::invokeMethod(this, "onNewFile", Qt::QueuedConnection);
		return;
	}

	if (name == "actionSaveEncode")
	{
		onEncryptSave();
		return;
	}
	if (name == "actionSaveDecode")
	{
		onDecryptSave();
		return;
	}

	if (name == "actionDebug")
	{
		injector.isScriptDebugModeEnable.store(pAction->isChecked(), std::memory_order_release);
		return;
	}
}

void ScriptEditor::onScriptStartMode()
{
	ui.mainToolBar->setUpdatesEnabled(false);

	ui.statusBar->setStyleSheet("color: rgb(255, 255, 255); background-color: rgb(134, 27, 45); border:none");
	usageLabel_->setBackgroundColor(QColor(134, 27, 45));
	lineLable_->setBackgroundColor(QColor(134, 27, 45));
	sizeLabel_->setBackgroundColor(QColor(134, 27, 45));
	indexLabel_->setBackgroundColor(QColor(134, 27, 45));
	eolLabel_->setBackgroundColor(QColor(134, 27, 45));

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
	ui.actionStop->setIcon(QIcon(":/image/icon_stop.svg"));
	ui.actionPause->setIcon(QIcon(":/image/icon_pause.svg"));
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

	createSpeedSpinBox();

	//禁止編輯
	ui.widget->setReadOnly(true);

	ui.mainToolBar->setUpdatesEnabled(true);
}

void ScriptEditor::onScriptStopMode()
{
	ui.mainToolBar->setUpdatesEnabled(false);

	ui.statusBar->setStyleSheet("color: rgb(255, 255, 255); background-color: rgb(64, 53, 130); border:none");
	usageLabel_->setBackgroundColor(QColor(64, 53, 130));
	lineLable_->setBackgroundColor(QColor(64, 53, 130));
	sizeLabel_->setBackgroundColor(QColor(64, 53, 130));
	indexLabel_->setBackgroundColor(QColor(64, 53, 130));
	eolLabel_->setBackgroundColor(QColor(64, 53, 130));

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
	ui.actionStop->setIcon(QIcon(":/image/icon_stopdisable.svg"));
	ui.actionPause->setIcon(QIcon(":/image/icon_pausedisable.svg"));
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

	createSpeedSpinBox();

	ui.widget->setReadOnly(false);

	ui.mainToolBar->setUpdatesEnabled(true);

	onAddForwardMarker(-1, false);
	onAddStepMarker(-1, false);

	qint64 currnetIndex = getIndex();
	forward_markers[currnetIndex].clear();
	step_markers[currnetIndex].clear();
}

void ScriptEditor::onScriptBreakMode()
{
	ui.mainToolBar->setUpdatesEnabled(false);

	ui.statusBar->setStyleSheet("color: rgb(255, 255, 255); background-color: rgb(66, 66, 66); border:none");
	usageLabel_->setBackgroundColor(QColor(66, 66, 66));
	lineLable_->setBackgroundColor(QColor(66, 66, 66));
	sizeLabel_->setBackgroundColor(QColor(66, 66, 66));
	indexLabel_->setBackgroundColor(QColor(66, 66, 66));
	eolLabel_->setBackgroundColor(QColor(66, 66, 66));

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
	ui.actionStop->setIcon(QIcon(":/image/icon_stop.svg"));
	ui.actionPause->setIcon(QIcon(":/image/icon_pausedisable.svg"));

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

	createSpeedSpinBox();

	//禁止編輯
	ui.widget->setReadOnly(true);

	ui.mainToolBar->setUpdatesEnabled(true);
}

void ScriptEditor::onScriptPauseMode()
{
	ui.mainToolBar->setUpdatesEnabled(false);

	ui.statusBar->setStyleSheet("color: rgb(255, 255, 255); background-color: rgb(66, 66, 66); border:none");
	usageLabel_->setBackgroundColor(QColor(66, 66, 66));
	lineLable_->setBackgroundColor(QColor(66, 66, 66));
	sizeLabel_->setBackgroundColor(QColor(66, 66, 66));
	indexLabel_->setBackgroundColor(QColor(66, 66, 66));
	eolLabel_->setBackgroundColor(QColor(66, 66, 66));

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
	ui.actionStop->setIcon(QIcon(":/image/icon_stop.svg"));
	ui.actionPause->setIcon(QIcon(":/image/icon_pausedisable.svg"));

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

	createSpeedSpinBox();

	//禁止編輯
	ui.widget->setReadOnly(true);

	ui.mainToolBar->setUpdatesEnabled(true);
}

void ScriptEditor::onAddForwardMarker(qint64 liner, bool b)
{
	qint64 currnetIndex = getIndex();
	setMark(CodeEditor::SymbolHandler::SYM_ARROW, forward_markers[currnetIndex], liner, b);
}

void ScriptEditor::onAddErrorMarker(qint64 liner, bool b)
{
	qint64 currnetIndex = getIndex();
	setMark(CodeEditor::SymbolHandler::SYM_TRIANGLE, error_markers[currnetIndex], liner, b);
}

void ScriptEditor::onAddStepMarker(qint64, bool b)
{
	qint64 currnetIndex = getIndex();
	if (!b)
	{
		ui.widget->setUpdatesEnabled(false);
		setMark(CodeEditor::SymbolHandler::SYM_STEP, step_markers[currnetIndex], -1, false);
		ui.widget->setUpdatesEnabled(true);
	}
	else
	{
		ui.widget->setUpdatesEnabled(false);
		setStepMarks();
		ui.widget->setUpdatesEnabled(true);
		emit SignalDispatcher::getInstance(currnetIndex).scriptBreaked();
	}
}

void ScriptEditor::onAddBreakMarker(qint64 liner, bool b)
{
	qint64 currnetIndex = getIndex();
	Injector& injector = Injector::getInstance(currnetIndex);
	do
	{
		if (liner == -1)
		{
			ui.widget->markerDeleteAll(CodeEditor::SymbolHandler::SYM_POINT);
			break;
		}

		QString currentScriptFileName = injector.currentScriptFileName;
		if (currentScriptFileName.isEmpty())
			break;

		if (b)
		{
			util::SafeHash<qint64, break_marker_t> markers = break_markers[currnetIndex].value(currentScriptFileName);
			break_marker_t bk = markers.value(liner);
			bk.line = liner;
			bk.content = ui.widget->text(liner);
			if (bk.content.simplified().isEmpty() || bk.content.simplified().indexOf("//") == 0 || bk.content.simplified().indexOf("/*") == 0)
				return;

			bk.maker = static_cast<qint64>(CodeEditor::SymbolHandler::SYM_POINT);

			markers.insert(liner, bk);
			break_markers[currnetIndex].insert(currentScriptFileName, markers);
			ui.widget->markerAdd(liner, CodeEditor::SymbolHandler::SYM_POINT);
		}
		else if (!b)
		{
			util::SafeHash<qint64, break_marker_t> markers = break_markers[currnetIndex].value(currentScriptFileName);
			if (markers.contains(liner))
			{
				markers.remove(liner);
				break_markers[currnetIndex].insert(currentScriptFileName, markers);
			}

			ui.widget->markerDelete(liner, CodeEditor::SymbolHandler::SYM_POINT);
		}
	} while (false);

	emit breakMarkInfoImport();
}

void ScriptEditor::onBreakMarkInfoImport()
{
	ui.treeWidget_breakList->setUpdatesEnabled(false);
	qint64 currnetIndex = getIndex();
	QList<QTreeWidgetItem*> trees = {};
	ui.treeWidget_breakList->clear();
	ui.treeWidget_breakList->setColumnCount(4);
	ui.treeWidget_breakList->setHeaderLabels(QStringList{ tr("CONTENT"),tr("COUNT"), tr("ROW"), tr("FILE") });
	const util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>> mks = break_markers[currnetIndex];
	TreeWidgetItem* item = nullptr;
	for (auto it = mks.cbegin(); it != mks.cend(); ++it)
	{
		QString fileName = it.key();
		const util::SafeHash<qint64, break_marker_t> mk = mks.value(fileName);
		for (const break_marker_t& bit : mk)
		{
			item = new TreeWidgetItem({ bit.content, util::toQString(bit.count), util::toQString(bit.line + 1), fileName });
			if (item == nullptr)
				continue;

			item->setIcon(0, QIcon(":/image/icon_breakpointenabled.svg"));
			trees.append(item);
		}
		ui.treeWidget_breakList->addTopLevelItems(trees);
	}

	ui.treeWidget_breakList->setUpdatesEnabled(true);
}

void ScriptEditor::onReloadScriptList()
{
	if (IS_LOADING) return;

	QStringList newScriptList = {};
	do
	{
		TreeWidgetItem* item = new TreeWidgetItem;
		if (item == nullptr)
			break;

		util::loadAllFileLists(item, util::applicationDirPath() + "/script/", &newScriptList, ":/image/icon_textfile.svg", ":/image/icon_folder.svg");

		scriptList_ = newScriptList;

		ui.treeWidget_scriptList->setUpdatesEnabled(false);

		ui.treeWidget_scriptList->clear();
		ui.treeWidget_scriptList->addTopLevelItem(item);
		//展開全部第一層
		ui.treeWidget_scriptList->topLevelItem(0)->setExpanded(true);

		//展開全部子層
		//for (qint64 i = 0; i < item->childCount(); ++i)
		//{
		//	ui.treeWidget_scriptList->expandItem(item->child(i));
		//}

		ui.treeWidget_scriptList->sortItems(0, Qt::AscendingOrder);

		ui.treeWidget_scriptList->setUpdatesEnabled(true);
	} while (false);
}

//全局變量列表
void ScriptEditor::onVarInfoImport(void* p, const QVariantHash& d, const QStringList& globalNames)
{
	ui.treeWidget_debuger_custom->setUpdatesEnabled(false);
	ui.treeWidget_debuger_custom->clear();
	QList<QTreeWidgetItem*> nodes = {};
	createTreeWidgetItems(reinterpret_cast<Parser*>(p), &nodes, d, globalNames);
	ui.treeWidget_debuger_custom->addTopLevelItems(nodes);
	ui.treeWidget_debuger_custom->setUpdatesEnabled(true);
}

void ScriptEditor::onEncryptSave()
{
#ifdef CRYPTO_H
	qint64 currnetIndex = getIndex();
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
	Injector& injector = Injector::getInstance(currnetIndex);
	if (crypto.encodeScript(injector.currentScriptFileName, password))
	{
		QString newFileName = injector.currentScriptFileName;
		newFileName.replace(util::SCRIPT_DEFAULT_SUFFIX, util::SCRIPT_PRIVATE_SUFFIX_DEFAULT);
		injector.currentScriptFileName = newFileName;
		ui.statusBar->showMessage(QString(tr("Encrypt script %1 saved")).arg(newFileName), 3000);
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currnetIndex);
		emit signalDispatcher.loadFileToTable(newFileName);
	}
	else
	{
		ui.statusBar->showMessage(tr("Encrypt script save failed"), 3000);
	}
#endif
}

void ScriptEditor::onDecryptSave()
{
#ifdef CRYPTO_H
	qint64 currnetIndex = getIndex();
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
	Injector& injector = Injector::getInstance(currnetIndex);
	if (crypto.decodeScript(injector.currentScriptFileName, content))
	{
		QString newFileName = injector.currentScriptFileName;
		newFileName.replace(util::SCRIPT_PRIVATE_SUFFIX_DEFAULT, util::SCRIPT_DEFAULT_SUFFIX);
		injector.currentScriptFileName = newFileName;
		ui.statusBar->showMessage(QString(tr("Decrypt script %1 saved")).arg(newFileName), 3000);
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currnetIndex);
		emit signalDispatcher.loadFileToTable(newFileName);
	}
	else
	{
		ui.statusBar->showMessage(tr("Decrypt password is incorrect"), 3000);
	}
#endif
}

static const QHash<qint64, QString> hashSolLuaType = {
	{ static_cast<qint64>(sol::type::none), QObject::tr("None") },
	{ static_cast<qint64>(sol::type::lua_nil), QObject::tr("Nil") },
	{ static_cast<qint64>(sol::type::string), QObject::tr("String") },
	{ static_cast<qint64>(sol::type::number), QObject::tr("Number") },
	{ static_cast<qint64>(sol::type::thread), QObject::tr("Thread") },
	{ static_cast<qint64>(sol::type::boolean), QObject::tr("Boolean") },
	{ static_cast<qint64>(sol::type::function), QObject::tr("Function") },
	{ static_cast<qint64>(sol::type::userdata), QObject::tr("Userdata") },
	{ static_cast<qint64>(sol::type::lightuserdata), QObject::tr("Lightuserdata") },
	{ static_cast<qint64>(sol::type::table), QObject::tr("Table") },
	{ static_cast<qint64>(sol::type::poly), QObject::tr("Poly") },
};

bool luaTableToTreeWidgetItem(QString field, TreeWidgetItem* pParentNode, const sol::table& t, qint64& depth)
{
	if (pParentNode == nullptr)
		return false;

	if (depth <= 0)
		return false;

	if (!t.valid())
		return false;

	if (t.empty())
		return false;

	--depth;

	TreeWidgetItem* pNode = nullptr;
	for (const auto& pair : t)
	{
		qint64 nKey = 0;
		QString key = "";
		QString value = "";
		QString varType = "";

		if (pair.first.is<qint64>())
		{
			key = QString("[%1]").arg(pair.first.as<qint64>());
		}
		else
			key = util::toQString(pair.first);

		if (pair.second.is<sol::table>())
		{
			varType = QObject::tr("Table");
			pNode = new TreeWidgetItem({ field, key, "", QString("(%1)").arg(varType) });
			if (pNode == nullptr)
				continue;

			if (luaTableToTreeWidgetItem(field, pNode, pair.second.as<sol::table>(), depth))
			{
				pNode->setIcon(0, QIcon(":/image/icon_class.svg"));
				pParentNode->addChild(pNode);
			}
			else
				delete pNode;
			continue;
		}
		else if (pair.second.is<std::string>())
		{
			varType = QObject::tr("String");
			value = QString("'%1'").arg(util::toQString(pair.second));
		}
		else if (pair.second.is<qint64>())
		{
			varType = QObject::tr("Int");
			value = util::toQString(pair.second.as<qint64>());
		}
		else if (pair.second.is<double>())
		{
			varType = QObject::tr("Double");
			value = util::toQString(pair.second.as<double>());
		}
		else if (pair.second.is<bool>())
		{
			varType = QObject::tr("Bool");
			value = util::toQString(pair.second.as<bool>());
		}
		else if (pair.second.is<sol::function>())
		{
			varType = QObject::tr("Function");
			value = QString("0x%1").arg(util::toQString(reinterpret_cast<qint64>(pair.second.as<sol::function>().pointer()), 16));
			continue;
		}
		else if (pair.second == sol::lua_nil)
		{
			varType = QObject::tr("Nil");
			value = "nil";
		}
		else
		{
			varType = hashSolLuaType.value(static_cast<qint64>(pair.second.get_type()), QObject::tr("Unknown"));
			value = QString("0x%1").arg(util::toQString(reinterpret_cast<qint64>(pair.second.pointer()), 16));
			continue;
		}

		if (varType.isEmpty())
			continue;

		pNode = new TreeWidgetItem({ field, key.isEmpty() ? util::toQString(nKey + 1) : key, value, QString("(%1)").arg(varType) });
		if (pNode == nullptr)
			continue;

		pNode->setIcon(0, QIcon(":/image/icon_field.svg"));
		pParentNode->addChild(pNode);
	}

	return true;
}

void ScriptEditor::createTreeWidgetItems(Parser* pparser, QList<QTreeWidgetItem*>* pTrees, const QHash<QString, QVariant>& d, const QStringList& globalNames)
{
	if (pTrees == nullptr)
		return;

	sol::state& lua_ = pparser->pLua_->getLua();

	QMap<QString, QVariant> map;
	TreeWidgetItem* pNode = nullptr;
	qint64 depth = kMaxLuaTableDepth;

	if (!d.isEmpty())
	{
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

			QString field = l.value(0) == "global" ? QObject::tr("GLOBAL") : QObject::tr("LOCAL");
			QString varName = l.value(1);
			QString varType;
			QString varValueStr;
			QVariant var = it.value();
			switch (var.type())
			{
			case QVariant::Int:
			{
				varType = QObject::tr("Int");
				varValueStr = util::toQString(var.toLongLong());
				break;
			}
			case QVariant::UInt:
			{
				varType = QObject::tr("UInt");
				varValueStr = util::toQString(var.toLongLong());
				break;
			}
			case QVariant::Double:
			{
				varType = QObject::tr("Double");
				varValueStr = util::toQString(var.toDouble());
				break;
			}
			case QVariant::String:
			{
				varValueStr = var.toString();
				if (varValueStr == "nil")
					varType = QObject::tr("Nil");
				else if (varValueStr.startsWith("{") && varValueStr.endsWith("}") && !varValueStr.startsWith("{:"))
				{
					varType = QObject::tr("Table");

					depth = kMaxLuaTableDepth;
					pparser->luaDoString(QString("_TMP = %1").arg(var.toString()));
					if (lua_["_TMP"].is<sol::table>())
					{
						pNode = new TreeWidgetItem(QStringList{ field, varName, "", QString("(%1)").arg(varType) });
						if (pNode == nullptr)
							continue;

						if (luaTableToTreeWidgetItem(field, pNode, lua_["_TMP"].get<sol::table>(), depth))
						{
							pNode->setIcon(0, QIcon(":/image/icon_class.svg"));
							pTrees->append(pNode);
						}
						else
							delete pNode;
					}
					lua_["_TMP"] = sol::lua_nil;
					continue;
				}
				else
					varType = QObject::tr("String");
				break;
			}
			case QVariant::Bool:
			{
				varType = QObject::tr("Bool");
				varValueStr = util::toQString(var.toBool());
				break;
			}
			case QVariant::LongLong:
			{
				varType = QObject::tr("LongLong");
				varValueStr = util::toQString(var.toLongLong());
				break;
			}
			case QVariant::ULongLong:
			{
				varType = QObject::tr("ULongLong");
				varValueStr = util::toQString(var.toULongLong());
				break;
			}
			default:
			{
				varType = QObject::tr("unknown");
				varValueStr = var.toString();
				break;
			}
			}

			pNode = new TreeWidgetItem({ field, varName, varValueStr, QString("(%1)").arg(varType) });
			if (pNode == nullptr)
				continue;
			pNode->setIcon(0, QIcon(":/image/icon_field.svg"));
			pTrees->append(pNode);
		}
	}

	for (const QString& it : globalNames)
	{
		const std::string name = it.toUtf8().constData();
		const sol::object o = lua_[name.c_str()];
		if (!o.valid())
			continue;

		const QString field = QObject::tr("GLOBAL");

		const QString varName = it;

		QString varType;

		QString varValueStr;

		if (o.is<sol::table>())
		{
			varType = QObject::tr("Table");
			pNode = new TreeWidgetItem(QStringList{ field, varName, "", QString("(%1)").arg(varType) });
			if (pNode == nullptr)
				continue;

			depth = kMaxLuaTableDepth;
			if (luaTableToTreeWidgetItem(field, pNode, o.as<sol::table>(), depth))
			{
				pNode->setIcon(0, QIcon(":/image/icon_class.svg"));
				pTrees->append(pNode);
			}
			else
				delete pNode;
			continue;
		}
		else if (o.is<std::string>())
		{
			varType = QObject::tr("String");
			varValueStr = QString("'%1'").arg(util::toQString(o.as<std::string>()));
		}
		else if (o.is<qint64>())
		{
			varType = QObject::tr("Int");
			varValueStr = util::toQString(o.as<qint64>());
		}
		else if (o.is<double>())
		{
			varType = QObject::tr("Double");
			varValueStr = util::toQString(o.as<double>());
		}
		else if (o.is<bool>())
		{
			varType = QObject::tr("Bool");
			varValueStr = util::toQString(o.as<bool>());
		}
		else if (o.is<sol::function>())
		{
			varType = QObject::tr("Function");
			varValueStr = QString("0x%1").arg(util::toQString(reinterpret_cast<qint64>(o.as<sol::function>().pointer()), 16));
			continue;
		}
		else if (o == sol::lua_nil)
		{
			varType = QObject::tr("Nil");
			varValueStr = "nil";
		}
		else
		{
			varType = hashSolLuaType.value(static_cast<qint64>(o.get_type()), QObject::tr("Unknown"));
			varValueStr = QString("0x%1").arg(util::toQString(reinterpret_cast<qint64>(o.pointer()), 16));
		}

		if (varType.isEmpty())
			continue;

		pNode = new TreeWidgetItem({ field, varName, varValueStr, QString("(%1)").arg(varType) });
		if (pNode == nullptr)
			continue;

		pNode->setIcon(0, QIcon(":/image/icon_field.svg"));
		pTrees->append(pNode);
	}
}