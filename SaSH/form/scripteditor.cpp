/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2024 All Rights Reserved.
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

#include <gamedevice.h>
#include "signaldispatcher.h"

#include <QSpinBox>

ScriptEditor::ScriptEditor(long long index, QWidget* parent)
	: QMainWindow(parent)
	, Indexer(index)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_QuitOnClose);
	setAttribute(Qt::WA_StyledBackground);

	qRegisterMetaType<QVector<long long>>();

	takeCentralWidget();

	setDockNestingEnabled(true);
	addDockWidget(Qt::LeftDockWidgetArea, ui.dockWidget_functionList);
	addDockWidget(Qt::RightDockWidgetArea, ui.dockWidget_scriptList);
	addDockWidget(Qt::BottomDockWidgetArea, ui.dockWidget_scriptFun);

	splitDockWidget(ui.dockWidget_functionList, ui.dockWidget_main, Qt::Horizontal);
	splitDockWidget(ui.dockWidget_main, ui.dockWidget_scriptList, Qt::Horizontal);
	splitDockWidget(ui.dockWidget_functionList, ui.dockWidget_autovar, Qt::Vertical);
	splitDockWidget(ui.dockWidget_main, ui.dockWidget_scriptFun, Qt::Vertical);
	ui.dockWidget_functionList->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	ui.dockWidget_main->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	ui.dockWidget_scriptList->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	ui.dockWidget_autovar->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	ui.dockWidget_sysvar->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
	ui.dockWidget_scriptFun->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

	tabifyDockWidget(ui.dockWidget_scriptFun, ui.dockWidget_des);
	tabifyDockWidget(ui.dockWidget_scriptFun, ui.dockWidget_mark);

	tabifyDockWidget(ui.dockWidget_autovar, ui.dockWidget_sysvar);

	ui.treeWidget_scriptList->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui.treeWidget_scriptList->resizeColumnToContents(1);
	ui.treeWidget_scriptList->header()->setSectionsClickable(true);
	connect(ui.treeWidget_scriptList->header(), &QHeaderView::sectionClicked, this, &ScriptEditor::onScriptTreeWidgetHeaderClicked, Qt::QueuedConnection);
	connect(ui.treeWidget_scriptList, &QTreeWidget::itemDoubleClicked, this, &ScriptEditor::onScriptTreeWidgetDoubleClicked, Qt::QueuedConnection);
	connect(ui.treeWidget_scriptList, &QTreeWidget::itemChanged, this, &ScriptEditor::onScriptTreeWidgetItemChanged, Qt::QueuedConnection);

	connect(ui.widget, &CodeEditor::findAllFinished, this, &ScriptEditor::onFindAllFinished, Qt::QueuedConnection);

	ui.treeWidget_functionList->sortItems(0, Qt::AscendingOrder);

	ui.listView_log->setTextElideMode(Qt::ElideNone);
	ui.listView_log->setResizeMode(QListView::Adjust);

	ui.widget->setIndex(index);

	ui.menuBar->setMinimumWidth(100);

	ui.mainToolBar->show();

	connect(ui.actionSave, &QAction::triggered, this, &ScriptEditor::onActionTriggered, Qt::QueuedConnection);
	connect(ui.actionSaveAs, &QAction::triggered, this, &ScriptEditor::onActionTriggered, Qt::QueuedConnection);
	connect(ui.actionDirectory, &QAction::triggered, this, &ScriptEditor::onActionTriggered, Qt::QueuedConnection);
	connect(ui.actionVSCode, &QAction::triggered, this, &ScriptEditor::onActionTriggered, Qt::QueuedConnection);
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
	connect(&signalDispatcher, &SignalDispatcher::breakMarkInfoImport, this, &ScriptEditor::onBreakMarkInfoImport, Qt::QueuedConnection);

	initStaticLabel();
	createScriptListContextMenu();

	init();
}

void ScriptEditor::init()
{
	long long index = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	GameDevice& gamedevice = GameDevice::getInstance(index);
	ui.listView_log->setModel(&gamedevice.scriptLogModel);

	onScriptStopMode();
	emit signalDispatcher.reloadScriptList();
	emit signalDispatcher.applyHashSettingsToUI();

	//ui.webEngineView->setUrl(QUrl("https://gitee.com/Bestkakkoii/sash/wikis/pages"));

	if (!gamedevice.currentScriptFileName.get().isEmpty() && QFile::exists(gamedevice.currentScriptFileName.get()))
	{
		emit signalDispatcher.loadFileToTable(gamedevice.currentScriptFileName.get());
	}

	util::Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
	if (config.isValid())
	{
		QString ObjectName = ui.widget->objectName();
		long long fontSize = config.read<long long>(objectName(), ObjectName, "FontSize");
		if (fontSize > 0)
		{
			QFont font = ui.widget->getOldFont();
			font.setPointSize(fontSize);
			ui.widget->setNewFont(font);
			ui.widget->zoomTo(3);
		}

		ObjectName = ui.listView_log->objectName();
		fontSize = config.read<long long>(objectName(), ObjectName, "FontSize");
		if (fontSize > 0)
		{
			QFont f = ui.listView_log->font();
			f.setPointSize(fontSize);
			ui.listView_log->setFont(f);
		}

		ObjectName = ui.textBrowser->objectName();
		fontSize = config.read<long long>(objectName(), ObjectName, "FontSize");
		if (fontSize > 0)
		{
			QFont font = ui.textBrowser->font();
			font.setPointSize(fontSize);
			ui.textBrowser->setFont(font);
		}
	}
}

void ScriptEditor::initStaticLabel()
{
	lineLable_ = q_check_ptr(new FastLabel(tr("row:%1").arg(1), QColor("#FFFFFF"), QColor(64, 53, 130), ui.statusBar));
	sash_assume(lineLable_ != nullptr);
	lineLable_->setFixedWidth(60);
	lineLable_->setTextColor(QColor(255, 255, 255));
	sizeLabel_ = q_check_ptr(new FastLabel("| " + tr("size:%1").arg(0), QColor("#FFFFFF"), QColor(64, 53, 130), ui.statusBar));
	sash_assume(sizeLabel_ != nullptr);
	sizeLabel_->setFixedWidth(60);
	sizeLabel_->setTextColor(QColor(255, 255, 255));
	indexLabel_ = q_check_ptr(new FastLabel("| " + tr("index:%1").arg(1), QColor("#FFFFFF"), QColor(64, 53, 130), ui.statusBar));
	sash_assume(indexLabel_ != nullptr);
	indexLabel_->setFixedWidth(60);
	indexLabel_->setTextColor(QColor(255, 255, 255));

	const QsciScintilla::EolMode mode = ui.widget->eolMode();
	const QString modeStr(mode == QsciScintilla::EolWindows ? "CRLF" : mode == QsciScintilla::EolUnix ? "  LF" : "  CR");
	eolLabel_ = q_check_ptr(new FastLabel(QString("| %1").arg(modeStr), QColor("#FFFFFF"), QColor(64, 53, 130), ui.statusBar));
	sash_assume(eolLabel_ != nullptr);
	eolLabel_->setFixedWidth(50);
	eolLabel_->setTextColor(QColor(255, 255, 255));

	usageLabel_ = q_check_ptr(new FastLabel(QString(tr("Usage: cpu: %1% | memory: %2MB / %3MB"))
		.arg(0).arg(0).arg(0), QColor("#FFFFFF"), QColor(64, 53, 130), ui.statusBar));
	sash_assume(usageLabel_ != nullptr);
	usageLabel_->setFixedWidth(350);
	usageLabel_->setTextColor(QColor(255, 255, 255));

	QLabel* spaceLabeRight = q_check_ptr(new QLabel("", ui.statusBar));
	sash_assume(spaceLabeRight != nullptr);
	spaceLabeRight->setFrameStyle(QFrame::NoFrame);
	spaceLabeRight->setFixedWidth(10);

	QLabel* spaceLabelMiddle = q_check_ptr(new QLabel("", ui.statusBar));
	sash_assume(spaceLabelMiddle != nullptr);
	spaceLabelMiddle->setFrameStyle(QFrame::NoFrame);
	spaceLabelMiddle->setFixedWidth(100);

	connect(&usageTimer_, &QTimer::timeout, this, [this]()
		{
			if (!isVisible())
				return;

			qreal cpuUsage = 0;
			qreal memoryUsage = 0;
			qreal memoryTotal = 0;

			if (util::monitorThreadResourceUsage(idleTime_, kernelTime_, userTime_, &cpuUsage, &memoryUsage, &memoryTotal))
			{
				usageLabel_->setText(QString(tr("Usage: cpu: %1% | memory: %2MB / %3MB"))
					.arg(util::toQString(cpuUsage))
					.arg(util::toQString(memoryUsage))
					.arg(util::toQString(memoryTotal)));
			}
		});

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
	pSpeedDescLabel_ = q_check_ptr(new QLabel(tr("Script speed:"), ui.mainToolBar));
	sash_assume(pSpeedDescLabel_ != nullptr);
	pSpeedDescLabel_->setAttribute(Qt::WA_StyledBackground);

	pSpeedSpinBox = q_check_ptr(new QSpinBox(ui.mainToolBar));
	sash_assume(pSpeedSpinBox != nullptr);
	pSpeedSpinBox->setStyleSheet("QLabel {color: #FAFAFA; font-size: 12pt;}");

	long long currentIndex = getIndex();
	pSpeedSpinBox->setRange(0, 10000);
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	long long value = gamedevice.getValueHash(util::kScriptSpeedValue);
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
	usageTimer_.stop();
	qDebug() << "~ScriptEditor";
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	gamedevice.IS_SCRIPT_EDITOR_OPENED.off();
}

void ScriptEditor::hideEvent(QHideEvent* e)
{
	util::FormSettingManager formSettingManager(this);
	formSettingManager.saveSettings();
	QMainWindow::hideEvent(e);
}

void ScriptEditor::showEvent(QShowEvent* e)
{
	setUpdatesEnabled(true);
	blockSignals(false);
	update();
	usageTimer_.start(1500);

	util::FormSettingManager formSettingManager(this);
	formSettingManager.loadSettings();

	setAttribute(Qt::WA_Mapped);
	QMainWindow::showEvent(e);

	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	gamedevice.IS_SCRIPT_EDITOR_OPENED.on();
}

void ScriptEditor::closeEvent(QCloseEvent* e)
{
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	gamedevice.IS_SCRIPT_EDITOR_OPENED.off();

	setUpdatesEnabled(false);
	blockSignals(true);

	util::FormSettingManager formSettingManager(this);
	formSettingManager.saveSettings();

	util::Config config(QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));

	if (!gamedevice.currentScriptFileName.get().isEmpty())
		config.write("Script", "LastModifyFile", gamedevice.currentScriptFileName.get());

	QString ObjectName = ui.widget->objectName();
	QFont font = ui.widget->getOldFont();
	config.write(objectName(), ObjectName, "FontSize", font.pointSize());

	ObjectName = ui.listView_log->objectName();
	font = ui.listView_log->font();
	config.write(objectName(), ObjectName, "FontSize", font.pointSize());

	ObjectName = ui.textBrowser->objectName();
	font = ui.textBrowser->font();
	config.write(objectName(), ObjectName, "FontSize", font.pointSize());

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
	long long indentLevel = 0;
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
	long long endIndex = -1;
	long long commandIndex = -1;
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
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (gamedevice.IS_SCRIPT_FLAG.get())
		return;

	const QString directoryName(util::applicationDirPath() + "/script");

	const QDir dir(directoryName);
	if (!dir.exists())
		dir.mkdir(directoryName);

	const QString fileName(gamedevice.currentScriptFileName.get());//當前路徑

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

	if (!fileName.endsWith(".lua"))
		content = formatCode(content);

	if (!util::writeFile(fileName, content))
		return;

	ui.statusBar->showMessage(QString(tr("Script %1 saved")).arg(fileName), 10000);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.addForwardMarker(-1, false);
	emit signalDispatcher.addErrorMarker(-1, false);
	emit signalDispatcher.addStepMarker(-1, false);

	gamedevice.forward_markers.clear();
	gamedevice.error_markers.clear();
	gamedevice.step_markers.clear();
	reshowBreakMarker();

	emit ui.comboBox_labels->clicked();
	emit ui.comboBox_functions->clicked();
	emit ui.widget->modificationChanged(true);
}

void ScriptEditor::loadFile(const QString& fileName)
{
	QString content;
	QString originalData;
	bool isPrivate = false;
	if (!util::readFile(fileName, &content, &isPrivate, &originalData))
		return;

	if (fileName.endsWith(util::SCRIPT_LUA_SUFFIX_DEFAULT))
	{
		ui.widget->setSuffix(util::SCRIPT_LUA_SUFFIX_DEFAULT);
	}
	else
	{
		ui.widget->setSuffix(util::SCRIPT_DEFAULT_SUFFIX);
	}

	bool isReadOnly = ui.widget->isReadOnly();
	if (isReadOnly)
		ui.widget->setReadOnly(false);

	int curLine = -1;
	int curIndex = -1;
	ui.widget->getCursorPosition(&curLine, &curIndex);

	int lineFrom = -1, indexFrom = -1, lineTo = -1, indexTo = -1;
	ui.widget->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);

	//紀錄滾動條位置
	long long scollValue = ui.widget->verticalScrollBar()->value();

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	if (!gamedevice.worker.isNull() && gamedevice.worker->getOnlineFlag())
		setWindowTitle(QString("[%1][%2] %3").arg(currentIndex).arg(gamedevice.worker->getCharacter().name).arg(fileName));
	else
		setWindowTitle(QString("[%1] %2").arg(currentIndex).arg(fileName));

	ui.widget->setUpdatesEnabled(false);

	ui.widget->setModified(false);
	ui.widget->selectAll();

	if (!isPrivate)
		ui.widget->replaceSelectedText(content);
	else
		ui.widget->replaceSelectedText(originalData);

	ui.widget->setUpdatesEnabled(true);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.addForwardMarker(-1, false);
	emit signalDispatcher.addErrorMarker(-1, false);
	emit signalDispatcher.addStepMarker(-1, false);

	gamedevice.forward_markers.clear();
	gamedevice.error_markers.clear();
	gamedevice.step_markers.clear();

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
	else if (!isReadOnly && isPrivate)
	{
		ui.widget->setReadOnly(true);
	}

	if (gamedevice.IS_SCRIPT_FLAG.get())
	{
		onScriptStartMode();
	}
}

void ScriptEditor::setContinue()
{
	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);

	emit signalDispatcher.addForwardMarker(-1, false);
	emit signalDispatcher.addErrorMarker(-1, false);
	emit signalDispatcher.addStepMarker(-1, false);

	gamedevice.forward_markers.clear();
	gamedevice.error_markers.clear();
	gamedevice.step_markers.clear();

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
	long long count = (static_cast<long long>(filepath.size()) - 1);
	for (long long i = count; i >= 0; i--) //QStringlist類filepath反向存著初始item的路徑
	{ //將filepath反向輸出，相應的加入’/‘
		if (filepath.value(i).isEmpty())
			continue;
		strpath += filepath.value(i);
		if (i != 0)
			strpath += "/";
	}

	strpath = util::applicationDirPath() + "/script/" + strpath;
	strpath.remove("*");

	QFileInfo info(strpath);
	QString suffix = "." + info.suffix();
	if (suffix.isEmpty())
		strpath += util::SCRIPT_DEFAULT_SUFFIX;
	if (suffix != util::SCRIPT_PRIVATE_SUFFIX_DEFAULT && suffix != util::SCRIPT_DEFAULT_SUFFIX)
		strpath.replace(suffix, util::SCRIPT_DEFAULT_SUFFIX);

	return strpath;
};

void ScriptEditor::createScriptListContextMenu()
{
	// Create the custom context menu
	QMenu* menu = q_check_ptr(new QMenu(this));
	sash_assume(menu != nullptr);
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

	long long currentIndex = getIndex();

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

	connect(renameAction, &QAction::triggered, this, [this, currentIndex]()
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

			QString newCurrentText = currentText;
			newCurrentText.remove(util::SCRIPT_DEFAULT_SUFFIX);

			//create a input dialog with same style as the main window
			QInputDialog inputDialog(this);
			inputDialog.setWindowFlags(windowFlags());
			inputDialog.setInputMode(QInputDialog::TextInput);
			inputDialog.setWindowTitle(tr("Rename"));
			inputDialog.setLabelText(tr("New name:"));
			inputDialog.setTextValue(newCurrentText);
			inputDialog.resize(300, 100);
			inputDialog.setOkButtonText(QObject::tr("ok"));
			inputDialog.setCancelButtonText(QObject::tr("cancel"));

			//show the dialog
			if (inputDialog.exec() != QDialog::Accepted)
				return;

			QString newName = inputDialog.textValue();
			if (newName.isEmpty())
				return;

			if (!newName.endsWith(util::SCRIPT_DEFAULT_SUFFIX))
				newName.append(util::SCRIPT_DEFAULT_SUFFIX);

			if (newName == currentText)
				return;

			QString newPath = currentPath;
			//only replace the last part of the path
			newPath.replace(currentText, newName);

			//rename the file
			QFile::rename(currentPath, newPath);

			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
			emit signalDispatcher.reloadScriptList();
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

void ScriptEditor::setMark(CodeEditor::SymbolHandler element, safe::hash<QString, safe::hash<long long, break_marker_t>>& hash, long long liner, bool b)
{
	do
	{
		if (liner == -1)
		{
			hash.clear();
			ui.widget->markerDeleteAll(element);
			break;
		}

		GameDevice& gamedevice = GameDevice::getInstance(getIndex());

		QString currentScriptFileName = gamedevice.currentScriptFileName.get();
		if (currentScriptFileName.isEmpty())
			break;


		safe::hash<long long, break_marker_t> markers = hash.value(currentScriptFileName);

		if (b)
		{
			break_marker_t bk = markers.value(liner);
			bk.line = liner;
			bk.content = ui.widget->text(liner);
			bk.maker = static_cast<long long>(element);
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

	long long maxliner = list.size();
	long long index = 1;

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);


	QString currentScriptFileName = gamedevice.currentScriptFileName.get();
	if (currentScriptFileName.isEmpty())
		return;

	safe::hash<long long, break_marker_t> markers = gamedevice.step_markers.value(currentScriptFileName);
	break_marker_t bk = {};

	for (long long i = 0; i < maxliner; ++i)
	{
		index = i + 1;
		bk = markers.value(index);
		bk.line = index;
		bk.content = list.value(i);
		bk.maker = static_cast<long long>(CodeEditor::SymbolHandler::SYM_STEP);
		markers.insert(index, bk);
	}

	gamedevice.step_markers.insert(currentScriptFileName, markers);
}

void ScriptEditor::reshowBreakMarker()
{
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	const safe::hash<QString, safe::hash<long long, break_marker_t>> mks = gamedevice.break_markers;

	QString currentScriptFileName = gamedevice.currentScriptFileName.get();
	if (currentScriptFileName.isEmpty())
		return;

	for (auto it = mks.cbegin(); it != mks.cend(); ++it)
	{
		QString fileName = it.key();
		if (fileName != currentScriptFileName)
			continue;

		const safe::hash<long long, break_marker_t> mk = mks.value(fileName);
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
	std::ignore = margin;
	long long mask = ui.widget->markersAtLine(line);

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
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	if (!gamedevice.IS_SCRIPT_FLAG.get())
		onSetStaticLabelLineText(line, index);
}

void ScriptEditor::on_widget_textChanged()
{
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	const QString text(ui.widget->text());
	if (scripts_.value(gamedevice.currentScriptFileName.get(), "") != text)
	{
		scripts_.insert(gamedevice.currentScriptFileName.get(), text);
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
	long long cur = ui.comboBox_labels->currentIndex();
	ui.comboBox_labels->clear();

	long long count = conlist.size();
	for (long long i = 0; i < count; ++i)
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
		long long line = var.toLongLong();
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
	long long cur = ui.comboBox_functions->currentIndex();
	ui.comboBox_functions->clear();

	long long count = conlist.size();
	for (long long i = 0; i < count; ++i)
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
		long long line = var.toLongLong();
		emit editorCursorPositionChanged(line, NULL);
	}
}

void ScriptEditor::on_treeWidget_functionList_itemDoubleClicked(QTreeWidgetItem* item, int column)
{
	std::ignore = column;
	//insert selected item text to widget

	if (!item)
		return;

	//如果存在子節點則直接返回
	if (item->childCount() > 0)
		return;

	QString str = item->text(0);
	if (str.isEmpty())
		return;

	GameDevice& gamedevice = GameDevice::getInstance(getIndex());

	if (gamedevice.worker.isNull())
	{
		ui.widget->insert(str + " ");
		return;
	}

	if (str == "button" || str == "input")
	{
		sa::dialog_t dialog = gamedevice.worker->currentDialog.get();
		QString npcName = gamedevice.worker->mapUnitHash.value(dialog.unitid).name;
		if (npcName.isEmpty())
			return;

		str = QString("%1('', '%2', %3)").arg(str).arg(npcName).arg(dialog.dialogid);
	}
	else if (str == "waitdlg")
	{
		sa::dialog_t dialog = gamedevice.worker->currentDialog.get();
		QString lineStr;
		for (const QString& s : dialog.linedatas)
		{
			if (s.size() > 3)
			{
				lineStr = s;
				break;
			}
		}
		str = QString("if not %1('%2', 5000), -1").arg(str).arg(lineStr);
	}
	else if (str == "waitsay")
	{
		str = QString("if not %1('', 5000), -1").arg(str);
	}
	else if (str == "waititem")
	{
		str = QString("if not %1('', '', 5000), -1").arg(str);
	}
	else if (str == "waitpet")
	{
		str = QString("if not %1('', 5000), -1").arg(str);
	}
	else if (str == "waitpos")
	{
		QPoint pos = gamedevice.worker->getPoint();
		str = QString("if not %1(%2, %3, 5000), -1").arg(str).arg(pos.x()).arg(pos.y());
	}
	else if (str == "learn")
	{
		sa::dialog_t dialog = gamedevice.worker->currentDialog.get();
		QString npcName = gamedevice.worker->mapUnitHash.value(dialog.unitid).name;
		if (npcName.isEmpty())
			return;

		str = QString("%1(0, 0, 0, '%2', %3)").arg(str).arg(npcName).arg(dialog.dialogid);
	}
	else if (str == "findpath" || str == "move")
	{
		QPoint pos = gamedevice.worker->getPoint();
		str = QString("%1(%2, %3)").arg(str).arg(pos.x()).arg(pos.y());
	}
	else if (str == "dir")
	{
		long long dir = gamedevice.worker->getDir();
		str = QString("%1(%2)").arg(str).arg(dir);
	}
	else if (str == "walkpos")
	{
		QPoint pos = gamedevice.worker->getPoint();
		str = QString("if not %1(%2, %3, 5000), -1").arg(str).arg(pos.x()).arg(pos.y());
	}
	else if (str == "w")
	{
		QPoint pos = gamedevice.worker->getPoint();
		long long dir = gamedevice.worker->getDir();
		const QString dirStr = "ABCDEFGH";
		if (dir < 0 || dir >= dirStr.size())
			return;
		str = QString("%1(%2, %3, '%4')").arg(str).arg(pos.x()).arg(pos.y()).arg(dirStr.at(dir));
	}
	else if (str == "waitmap")
	{
		long long floor = gamedevice.worker->getFloor();
		str = QString("if not %1(%2, 5000), +2").arg(str).arg(floor);
	}
	else if (str == "sleep")
	{
		str = QString("%1(500)").arg(str);
	}
	else if (str == "function")
	{
		str = QString("%1 myfun()\n    \nend").arg(str);
	}
	else if (str == "for")
	{
		str = QString("%1 i = 1, 10, 1\n    \nend").arg(str);
	}
	else if (str == "findnpc")
	{
		QPoint pos = gamedevice.worker->getPoint();
		QList<sa::map_unit_t> units = gamedevice.worker->mapUnitHash.values();
		auto getdis = [&pos](QPoint p)
			{
				//歐幾里得
				return sqrt(pow(pos.x() - p.x(), 2) + pow(pos.y() - p.y(), 2));
			};

		std::sort(units.begin(), units.end(), [&getdis](const sa::map_unit_t& a, const sa::map_unit_t& b)
			{
				return getdis(a.p) < getdis(b.p);
			});

		if (units.isEmpty())
			return;
		sa::map_unit_t unit = units.first();
		while (unit.objType != sa::kObjectNPC)
		{
			units.removeFirst();
			if (units.isEmpty())
				return;
			unit = units.first();
		}

		//移动至NPC 参数1:NPC名称, 参数2:NPC暱称, 参数3:东坐标, 参数4:南坐标, 参数5:超时时间, 参数6:错误跳转
		str = QString("%1('%2', '%3', %4, %5, 10000, 50, true)").arg(str).arg(unit.name).arg(unit.freeName).arg(unit.p.x()).arg(unit.p.y());

	}
	else if (str == "findnpc with mod")
	{
		QPoint pos = gamedevice.worker->getPoint();
		QList<sa::map_unit_t> units = gamedevice.worker->mapUnitHash.values();
		auto getdis = [&pos](QPoint p)
			{
				//歐幾里得
				return sqrt(pow(pos.x() - p.x(), 2) + pow(pos.y() - p.y(), 2));
			};

		std::sort(units.begin(), units.end(), [&getdis](const sa::map_unit_t& a, const sa::map_unit_t& b)
			{
				return getdis(a.p) < getdis(b.p);
			});

		if (units.isEmpty())
			return;
		sa::map_unit_t unit = units.first();
		while (unit.objType != sa::kObjectNPC)
		{
			units.removeFirst();
			if (units.isEmpty())
				return;
			unit = units.first();
		}

		//移动至NPC 参数1:NPC名称, 参数2:NPC暱称, 参数3:东坐标, 参数4:南坐标, 参数5:超时时间, 参数6:错误跳转
		str = QString("findnpc(%1, '', %2, %3, 10000, 50, true)").arg(unit.modelid).arg(unit.p.x()).arg(unit.p.y());
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
			QTextDocument* pdoc = document_.value(str).get();
			if (nullptr != pdoc)
			{
				ui.textBrowser->setUpdatesEnabled(false);
				ui.textBrowser->setDocument(pdoc);
				ui.textBrowser->setUpdatesEnabled(true);
				break;
			}
			else
			{
				document_.remove(str);
			}
		}

		QString mdFullPath = util::applicationDirPath();

		QDir dir(mdFullPath);
		if (!dir.exists())
			break;

		QStringList result;
		//以此目錄為起點遍歷查找所有文件名包含 \'str\' 的 .MD 文件
		util::searchFiles(mdFullPath, QString(R"('%1')").arg(str), ".md", &result, true);
		if (result.isEmpty())
			return;

		//按文本長度排序 由短到長 
		QCollator collator = util::getCollator();
		std::sort(result.begin(), result.end(), collator);

		QString markdownText = result.join("\n---\n");

		std::unique_ptr<QTextDocument> pdoc(q_check_ptr(new QTextDocument()));
		sash_assume(pdoc != nullptr);
		if (nullptr == pdoc)
			break;

		pdoc->setMarkdown(markdownText);

		ui.textBrowser->setUpdatesEnabled(false);
		ui.textBrowser->setDocument(pdoc.get());
		ui.textBrowser->setUpdatesEnabled(true);
		document_.insert(str, std::move(pdoc));
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
	constexpr long long padding = 20; // 可以根據需要調整
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
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	if (item->text(2).isEmpty())
		return;

	if (gamedevice.IS_SCRIPT_FLAG.get())
		return;

	if (item->text(3).isEmpty())
		return;

	gamedevice.currentScriptFileName.set(item->text(3));
	loadFile(item->text(3));
	long long line = item->text(2).toLongLong();
	QString text = ui.widget->text(line - 1);
	ui.widget->setSelection(line - 1, 0, line - 1, text.length());
	ui.widget->ensureLineVisible(line - 1);
}

void findTreeItem(QTreeWidget* tree, const QString& qsFilter)
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
}

//查找命令
void ScriptEditor::on_lineEdit_searchFunction_textChanged(const QString& text)
{
	if (!text.isEmpty())
		findTreeItem(ui.treeWidget_functionList, text);
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

//查找變量
void ScriptEditor::on_lineEdit_searchVariable_textChanged(const QString& text)
{
	if (!text.isEmpty())
		findTreeItem(ui.treeWidget_debuger_custom, text);
	else
	{
		QTreeWidgetItemIterator it(ui.treeWidget_debuger_custom);
		while (*it)
		{
			(*it)->setHidden(false);
			++it;
		}
	}
}

//查找腳本
void ScriptEditor::on_lineEdit_searchScript_textChanged(const QString& text)
{
	if (!text.isEmpty())
		findTreeItem(ui.treeWidget_scriptList, text);
	else
	{
		QTreeWidgetItemIterator it(ui.treeWidget_scriptList);
		while (*it)
		{
			(*it)->setHidden(false);
			++it;
		}
	}
}

//查找系統變量
void ScriptEditor::on_lineEdit_searchSystemVariable_textChanged(const QString& text)
{
	if (!text.isEmpty())
		findTreeItem(ui.treeWidget_debuger_sys, text);
	else
	{
		QTreeWidgetItemIterator it(ui.treeWidget_debuger_sys);
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
	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (!gamedevice.worker.isNull() && gamedevice.worker->getOnlineFlag())
	{
		QString title = gamedevice.currentScriptFileName.get();
		QString newTitle = QString("[%1][%2] %3").arg(currentIndex).arg(gamedevice.worker->getCharacter().name).arg(title);
		setWindowTitle(newTitle);
	}

	ui.listView_log->setModel(&gamedevice.scriptLogModel);

	if (pSpeedSpinBox != nullptr)
		pSpeedSpinBox->setValue(gamedevice.getValueHash(util::kScriptSpeedValue));

	ui.actionDebug->setChecked(gamedevice.IS_SCRIPT_DEBUG_ENABLE.get());
}

void ScriptEditor::onWidgetModificationChanged(bool changed)
{
	if (!changed) return;
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	scripts_.insert(gamedevice.currentScriptFileName.get(), ui.widget->text());
}

void ScriptEditor::onScriptTreeWidgetDoubleClicked(QTreeWidgetItem* item, int column)
{
	if (!item)
		return;

	std::ignore = column;
	if (item->text(0).isEmpty())
		return;

	do
	{
		long long currnetIndex = getIndex();
		GameDevice& gamedevice = GameDevice::getInstance(currnetIndex);
		if (gamedevice.IS_SCRIPT_FLAG.get())
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
		long long count = (static_cast<long long>(filepath.size()) - 1);
		for (long long i = count; i >= 0; i--) //QStringlist類filepath反向存著初始item的路徑
		{ //將filepath反向輸出，相應的加入’/‘
			if (filepath.value(i).isEmpty())
				continue;
			strpath += filepath.value(i);
			if (i != 0)
				strpath += "/";
		}

		strpath = util::applicationDirPath() + "/script/" + strpath;
		strpath.remove("*");

		QFileInfo fileinfo(strpath);
		if (!fileinfo.isFile())
			break;

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currnetIndex);
		emit signalDispatcher.addForwardMarker(-1, false);
		emit signalDispatcher.addErrorMarker(-1, false);
		emit signalDispatcher.addStepMarker(-1, false);

		gamedevice.forward_markers.clear();
		gamedevice.error_markers.clear();
		gamedevice.step_markers.clear();

		if (!gamedevice.IS_SCRIPT_FLAG.get())
			gamedevice.currentScriptFileName.set(strpath);

		emit signalDispatcher.loadFileToTable(strpath);

	} while (false);
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
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	if (line < 0)
		line = 1;

	if (!gamedevice.IS_SCRIPT_DEBUG_ENABLE.get())
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

	long long currentIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	gamedevice.setValueHash(util::kScriptSpeedValue, value);
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.applyHashSettingsToUI();
}

void ScriptEditor::onNewFile()
{
	long long currnetIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currnetIndex);
	long long num = 1;
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
	long long currnetIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currnetIndex);
	QString name = pAction->objectName();
	if (name.isEmpty())
		return;

	GameDevice& gamedevice = GameDevice::getInstance(currnetIndex);

	if (name == "actionSave")
	{
		gamedevice.scriptLogModel.clear();

		fileSave(ui.widget->text());
		emit signalDispatcher.loadFileToTable(gamedevice.currentScriptFileName.get());
		return;
	}

	if (name == "actionStart")
	{
		if (gamedevice.step_markers.size() == 0 && !gamedevice.IS_SCRIPT_FLAG.get())
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
		QString directoryName(util::applicationDirPath() + "/script");
		const QDir dir(directoryName);
		if (!dir.exists())
			dir.mkdir(directoryName);

		QString path;
		if (!util::fileDialogShow("*.txt", QFileDialog::AcceptSave, &path, this))
			return;

		if (path.isEmpty() || !util::writeFile(path, ui.widget->text()))
		{
			return;
		}

		QFileInfo fileInfo(path);
		directoryName = fileInfo.absolutePath();

		QDesktopServices::openUrl(QUrl::fromLocalFile(directoryName));
		emit signalDispatcher.reloadScriptList();
		return;
	}

	if (name == "actionDirectory")
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(util::applicationDirPath() + "/script"));
		return;
	}

	if (name == "actionVSCode")
	{
		//-r dir scriptpath
		QStringList args = { "code", "-r" };
		QString dir = util::applicationDirPath() + "/script";
		dir.replace("/", "\\");
		QString scriptpath = gamedevice.currentScriptFileName.get();
		scriptpath.replace("/", "\\");

		if (!gamedevice.currentScriptFileName.get().isEmpty())
			args << dir << scriptpath;
		else
			args << dir;

		QString command = args.join(" ");

		//write temp bat that can delete self after run
		QString batText = QString("@echo off\n%1\n").arg(command);
		util::asyncRunBat(QDir::tempPath(), batText);
		return;
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
		if (gamedevice.worker.isNull())
			return;

		if (gamedevice.isValid())
			gamedevice.setEnableHash(util::kLogBackEnable, true);
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
		gamedevice.IS_SCRIPT_DEBUG_ENABLE.set(pAction->isChecked());
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
	ui.actionVSCode->setEnabled(true);
	ui.mainToolBar->addAction(ui.actionLogback);
	ui.mainToolBar->addAction(ui.actionSave);
	ui.mainToolBar->addAction(ui.actionNew);
	ui.mainToolBar->addAction(ui.actionSaveAs);
	ui.mainToolBar->addAction(ui.actionDirectory);
	ui.mainToolBar->addAction(ui.actionVSCode);

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
	ui.mainToolBar->addAction(ui.actionVSCode);

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

	long long currnetIndex = getIndex();

	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	gamedevice.forward_markers.clear();
	gamedevice.step_markers.clear();
}

void ScriptEditor::onScriptBreakMode()
{
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	if (!gamedevice.IS_SCRIPT_FLAG.get())
		return;

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
	ui.mainToolBar->addAction(ui.actionVSCode);

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
	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	if (!gamedevice.IS_SCRIPT_FLAG.get())
		return;

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
	ui.mainToolBar->addAction(ui.actionVSCode);

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

void ScriptEditor::onAddForwardMarker(long long liner, bool b)
{
	long long currnetIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currnetIndex);
	setMark(CodeEditor::SymbolHandler::SYM_ARROW, gamedevice.forward_markers, liner, b);
}

void ScriptEditor::onAddErrorMarker(long long liner, bool b)
{
	long long currnetIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currnetIndex);
	setMark(CodeEditor::SymbolHandler::SYM_TRIANGLE, gamedevice.error_markers, liner, b);
}

void ScriptEditor::onAddStepMarker(long long, bool b)
{
	long long currnetIndex = getIndex();
	if (!b)
	{
		ui.widget->setUpdatesEnabled(false);
		GameDevice& gamedevice = GameDevice::getInstance(currnetIndex);
		setMark(CodeEditor::SymbolHandler::SYM_STEP, gamedevice.step_markers, -1, false);
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

void ScriptEditor::onAddBreakMarker(long long liner, bool b)
{
	long long currnetIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currnetIndex);
	do
	{
		if (liner == -1)
		{
			ui.widget->markerDeleteAll(CodeEditor::SymbolHandler::SYM_POINT);
			break;
		}

		QString currentScriptFileName = gamedevice.currentScriptFileName.get();
		if (currentScriptFileName.isEmpty())
			break;

		if (b)
		{
			safe::hash<long long, break_marker_t> markers = gamedevice.break_markers.value(currentScriptFileName);
			break_marker_t bk = markers.value(liner);
			bk.line = liner;
			bk.content = ui.widget->text(liner);
			if (bk.content.simplified().isEmpty() || bk.content.simplified().indexOf("//") == 0 || bk.content.simplified().indexOf("--[[") == 0)
				return;

			bk.maker = static_cast<long long>(CodeEditor::SymbolHandler::SYM_POINT);

			markers.insert(liner, bk);
			gamedevice.break_markers.insert(currentScriptFileName, markers);
			ui.widget->markerAdd(liner, CodeEditor::SymbolHandler::SYM_POINT);
		}
		else if (!b)
		{
			safe::hash<long long, break_marker_t> markers = gamedevice.break_markers.value(currentScriptFileName);
			if (markers.contains(liner))
			{
				markers.remove(liner);
				gamedevice.break_markers.insert(currentScriptFileName, markers);
			}

			ui.widget->markerDelete(liner, CodeEditor::SymbolHandler::SYM_POINT);
		}
	} while (false);

	emit breakMarkInfoImport();
}

void ScriptEditor::onBreakMarkInfoImport()
{
	ui.treeWidget_breakList->setUpdatesEnabled(false);
	long long currnetIndex = getIndex();
	GameDevice& gamedevice = GameDevice::getInstance(currnetIndex);
	QList<QTreeWidgetItem*> trees = {};
	ui.treeWidget_breakList->clear();
	ui.treeWidget_breakList->setColumnCount(4);
	ui.treeWidget_breakList->setHeaderLabels(QStringList{ tr("CONTENT"),tr("COUNT"), tr("ROW"), tr("FILE") });
	const safe::hash<QString, safe::hash<long long, break_marker_t>> mks = gamedevice.break_markers;
	TreeWidgetItem* item = nullptr;
	for (auto it = mks.cbegin(); it != mks.cend(); ++it)
	{
		QString fileName = it.key();
		const safe::hash<long long, break_marker_t> mk = mks.value(fileName);
		for (const break_marker_t& bit : mk)
		{
			item = q_check_ptr(new TreeWidgetItem({ bit.content, util::toQString(bit.count), util::toQString(bit.line + 1), fileName }));
			sash_assume(item != nullptr);
			if (item == nullptr)
				continue;

			item->setIcon(0, QIcon(":/image/icon_breakpointenabled.svg"));
			trees.append(item);
		}
		ui.treeWidget_breakList->addTopLevelItems(trees);
	}

	ui.treeWidget_breakList->setUpdatesEnabled(true);
}

//重新加載腳本列表
void ScriptEditor::onReloadScriptList()
{
	QStringList newScriptList = {};
	TreeWidgetItem* item = q_check_ptr(new TreeWidgetItem());
	sash_assume(item != nullptr);
	if (nullptr == item)
		return;

	util::loadAllFileLists(
		item,
		util::applicationDirPath() + "/script/", &newScriptList,
		":/image/icon_textfile.svg", ":/image/icon_folder.svg");

	scriptList_ = newScriptList;
	ui.treeWidget_scriptList->clear();
	ui.treeWidget_scriptList->addTopLevelItem(item);

	item->setExpanded(true);

	ui.treeWidget_scriptList->sortItems(0, Qt::AscendingOrder);
}

void ScriptEditor::onFindAllFinished(const QString& expr, const QVariant& varmap)
{
	if (varmap.isNull())
		return;

	//line, text|index
	QMap<long long, QVariantList> resultLineTexts = varmap.value<QMap<long long, QVariantList>>();
	if (resultLineTexts.isEmpty())
		return;

	//new a QDockWidget and dock to bottom
	if (nullptr == pDockWidgetFindAll_)
	{
		pDockWidgetFindAll_ = q_check_ptr(new QDockWidget(this));
		sash_assume(pDockWidgetFindAll_ != nullptr);
		if (nullptr == pDockWidgetFindAll_)
			return;

		pDockWidgetFindAll_->setAttribute(Qt::WA_StyledBackground);

		pTreeWidgetFindAll_ = q_check_ptr(new TreeWidget(pDockWidgetFindAll_));
		sash_assume(pTreeWidgetFindAll_ != nullptr);
		if (nullptr == pTreeWidgetFindAll_)
			return;

		pTreeWidgetFindAll_->setAttribute(Qt::WA_StyledBackground);

		pDockWidgetFindAll_->setWidget(pTreeWidgetFindAll_);
		pDockWidgetFindAll_->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);

		pTreeWidgetFindAll_->setColumnCount(3);// text|line|index
		pTreeWidgetFindAll_->setHeaderLabels(QStringList{ tr("TEXT"), tr("FILE"), tr("LINE"), tr("INDEX") });

		connect(pTreeWidgetFindAll_, &TreeWidget::itemDoubleClicked, this, &ScriptEditor::onFindAllDoubleClicked);
	}

	pDockWidgetFindAll_->setWindowTitle(QString("%1 \"%2\"").arg(tr("Find")).arg(expr));

	if (!pDockWidgetFindAll_->isVisible())
	{
		pDockWidgetFindAll_->setAllowedAreas(Qt::BottomDockWidgetArea);
		pDockWidgetFindAll_->show();

		//tab with log dockwidget
		tabifyDockWidget(ui.dockWidget_scriptFun, pDockWidgetFindAll_);
	}

	pTreeWidgetFindAll_->setUpdatesEnabled(false);
	pTreeWidgetFindAll_->clear();

	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	QString currentScriptFileName = gamedevice.currentScriptFileName.get();
	QFileInfo fileInfo(currentScriptFileName);
	QString fileName = fileInfo.fileName();

	TreeWidgetItem* parentNode = q_check_ptr(new TreeWidgetItem(QStringList{ QString("%1 (%2)").arg(fileName).arg(resultLineTexts.size()) }));
	sash_assume(nullptr != parentNode);
	if (nullptr == parentNode)
		return;

	//add items
	for (auto it = resultLineTexts.cbegin(); it != resultLineTexts.cend(); ++it)
	{
		long long line = it.key();
		QVariantList texts = it.value();
		if (texts.size() != 2)
			continue;

		QString text = texts.value(0).toString();
		long long index = texts.value(1).toLongLong();

		TreeWidgetItem* item = q_check_ptr(new TreeWidgetItem({ text, fileName, util::toQString(line + 1), util::toQString(index) }));
		sash_assume(nullptr != item);
		if (nullptr == item)
			continue;

		//set user data as original text
		item->setData(0, Qt::UserRole, expr);
		parentNode->addChild(item);
	}

	pTreeWidgetFindAll_->addTopLevelItem(parentNode);
	pTreeWidgetFindAll_->expandAll();
	pTreeWidgetFindAll_->setUpdatesEnabled(true);
}

void ScriptEditor::onFindAllDoubleClicked(QTreeWidgetItem* item, int column)
{
	if (nullptr == item)
		return;

	//col 0 = text, col 2 = line, col 3 = index
	QString expr = item->text(0);
	QString fileName = item->text(1);
	QString lineStr = item->text(2);
	QString indexStr = item->text(3);
	long long line = lineStr.toLongLong() - 1;
	long long index = indexStr.toLongLong();
	if (line < 0 || index < 0)
		return;

	QString originalText = item->data(0, Qt::UserRole).toString();

	ui.widget->ensureLineVisible(line);

	ui.widget->setUpdatesEnabled(false);
	ui.widget->setSelection(line, index, line, index + originalText.size());
	ui.widget->setCaretLineBackgroundColor(QColor(71, 71, 71));//光標所在行背景顏色
	ui.widget->setUpdatesEnabled(true);
}

//變量列表
void ScriptEditor::onVarInfoImport(void* p, const QVariantHash& d, const QStringList& globalNames)
{
	createTreeWidgetItems(ui.treeWidget_debuger_sys, ui.treeWidget_debuger_custom, reinterpret_cast<Parser*>(p), d, globalNames);
}

void ScriptEditor::onEncryptSave()
{
#ifdef CRYPTO_H
	long long currnetIndex = getIndex();
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
		ui.statusBar->showMessage(tr("Encrypt password can not be empty"), 5000);
		return;
	}

	Crypto crypto;
	GameDevice& gamedevice = GameDevice::getInstance(currnetIndex);
	if (crypto.encodeScript(gamedevice.currentScriptFileName.get(), password))
	{
		QString newFileName = gamedevice.currentScriptFileName.get();
		newFileName.replace(util::SCRIPT_DEFAULT_SUFFIX, util::SCRIPT_PRIVATE_SUFFIX_DEFAULT);
		gamedevice.currentScriptFileName.set(newFileName);
		ui.statusBar->showMessage(QString(tr("Encrypt script %1 saved")).arg(newFileName), 5000);
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currnetIndex);
		emit signalDispatcher.loadFileToTable(newFileName);
		emit signalDispatcher.reloadScriptList();
	}
	else
	{
		ui.statusBar->showMessage(tr("Encrypt script save failed"), 5000);
	}
#endif
}

void ScriptEditor::onDecryptSave()
{
#ifdef CRYPTO_H
	long long currnetIndex = getIndex();
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
		ui.statusBar->showMessage(tr("Decrypt password can not be empty"), 5000);
		return;
	}

	Crypto crypto;
	QString content;
	GameDevice& gamedevice = GameDevice::getInstance(currnetIndex);
	QString currentScriptFileName = gamedevice.currentScriptFileName.get();
	if (crypto.decodeScript(currentScriptFileName, password, content))
	{
		gamedevice.currentScriptFileName.set(currentScriptFileName);
		ui.statusBar->showMessage(QString(tr("Decrypt script %1 saved")).arg(currentScriptFileName), 5000);
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currnetIndex);
		emit signalDispatcher.loadFileToTable(currentScriptFileName);
		emit signalDispatcher.reloadScriptList();
	}
	else
	{
		ui.statusBar->showMessage(tr("Decrypt password is incorrect"), 5000);
	}
#endif
}

static const QHash<long long, QString> hashSolLuaType = {
	{ static_cast<long long>(sol::type::none), QObject::tr("None") },
	{ static_cast<long long>(sol::type::lua_nil), QObject::tr("Nil") },
	{ static_cast<long long>(sol::type::string), QObject::tr("String") },
	{ static_cast<long long>(sol::type::number), QObject::tr("Number") },
	{ static_cast<long long>(sol::type::thread), QObject::tr("Thread") },
	{ static_cast<long long>(sol::type::boolean), QObject::tr("Boolean") },
	{ static_cast<long long>(sol::type::function), QObject::tr("Function") },
	{ static_cast<long long>(sol::type::userdata), QObject::tr("Userdata") },
	{ static_cast<long long>(sol::type::lightuserdata), QObject::tr("Lightuserdata") },
	{ static_cast<long long>(sol::type::table), QObject::tr("Table") },
	{ static_cast<long long>(sol::type::poly), QObject::tr("Poly") },
};

bool luaTableToTreeWidgetItem(QString field, TreeWidgetItem* pParentNode, const sol::table& t, long long& depth, const QString& fieldStr)
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
		long long nKey = 0;
		QString key = "";
		QString value = "";
		QString varType = "";

		if (pair.first.is<long long>())
		{
			key = QString("[%1]").arg(pair.first.as<long long>());
		}
		else
			key = util::toQString(pair.first);

		if (pair.second.is<sol::table>() && !pair.second.is<sol::userdata>() && !pair.second.is<sol::function>() && !pair.second.is<sol::lightuserdata>())
		{
			varType = QObject::tr("Table");
			QStringList treeTexts = { field, key, "", QString("(%1)").arg(varType) };
			pNode = q_check_ptr(new TreeWidgetItem(treeTexts));
			sash_assume(pNode != nullptr);
			if (pNode == nullptr)
				continue;

			if (luaTableToTreeWidgetItem(field, pNode, pair.second.as<sol::table>(), depth, fieldStr))
			{
				pNode->setData(0, Qt::UserRole, fieldStr);
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
		else if (pair.second.is<long long>())
		{
			varType = QObject::tr("Int");
			value = util::toQString(pair.second.as<long long>());
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
			value = QString("0x%1").arg(util::toQString(reinterpret_cast<long long>(pair.second.as<sol::function>().pointer()), 16));
			continue;
		}
		else if (pair.second == sol::lua_nil)
		{
			varType = QObject::tr("Nil");
			value = "nil";
		}
		else
		{
			varType = hashSolLuaType.value(static_cast<long long>(pair.second.get_type()), QObject::tr("Unknown"));
			value = QString("0x%1").arg(util::toQString(reinterpret_cast<long long>(pair.second.pointer()), 16));
			continue;
		}

		if (varType.isEmpty())
			continue;

		QStringList treeTexts = { field, key.isEmpty() ? util::toQString(nKey + 1) : key, value, QString("(%1)").arg(varType) };
		pNode = q_check_ptr(new TreeWidgetItem(treeTexts));
		sash_assume(pNode != nullptr);
		if (pNode == nullptr)
			continue;

		pNode->setData(0, Qt::UserRole, fieldStr);
		pNode->setIcon(0, QIcon(":/image/icon_field.svg"));
		pParentNode->addChild(pNode);
	}

	return true;
}

void ScriptEditor::createTreeWidgetItems(TreeWidget* widgetSystem, TreeWidget* widgetCustom, Parser* pparser, const QHash<QString, QVariant>& d, const QStringList& globalNames)
{
	QMutexLocker locker(&varListMutex_);

	if (pparser == nullptr)
		return;

	if (nullptr == pparser->pLua_)
		return;

	if (widgetSystem == nullptr || widgetCustom == nullptr)
		return;

	QList<QTreeWidgetItem*> nodesSystem = {};
	QList<QTreeWidgetItem*> nodesCustom = {};

	widgetSystem->blockSignals(true);
	widgetCustom->blockSignals(true);

	widgetSystem->clear();
	widgetCustom->clear();

	sol::state& lua = pparser->pLua_->getLua();

	QMap<QString, QVariant> map;
	TreeWidgetItem* pNode = nullptr;
	long long depth = kMaxLuaTableDepth;

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
					pparser->luaDoString(QString("__TMP = %1").arg(var.toString()));
					if (lua["__TMP"].is<sol::table>())
					{
						QStringList treeTexts = { field, varName, "", QString("(%1)").arg(varType) };
						pNode = q_check_ptr(new TreeWidgetItem(treeTexts));
						sash_assume(pNode != nullptr);
						if (pNode == nullptr)
							continue;

						if (luaTableToTreeWidgetItem(field, pNode, lua["__TMP"].get<sol::table>(), depth, "local"))
						{
							pNode->setData(0, Qt::UserRole, QString("local"));
							pNode->setText(2, QString("(%1)").arg(pNode->childCount()));
							pNode->setIcon(0, QIcon(":/image/icon_class.svg"));
							nodesCustom.append(pNode);
						}
						else
							delete pNode;
					}

					lua["__TMP"] = sol::lua_nil;
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

			QStringList treeTexts = { field, varName, varValueStr, QString("(%1)").arg(varType) };
			pNode = q_check_ptr(new TreeWidgetItem(treeTexts));
			sash_assume(pNode != nullptr);
			if (pNode == nullptr)
				continue;

			pNode->setData(0, Qt::UserRole, QString("local"));
			pNode->setIcon(0, QIcon(":/image/icon_field.svg"));
			nodesCustom.append(pNode);
		}
	}

	for (const QString& it : globalNames)
	{
		const std::string name(util::toConstData(it));
		const sol::object o = lua[name.c_str()];
		if (!o.valid())
			continue;

		const QString field = QObject::tr("GLOBAL");

		const QString varName = it;

		QString varType;

		QString varValueStr;

		if (o.is<sol::table>() && !o.is<sol::userdata>() && !o.is<sol::function>() && !o.is<sol::lightuserdata>())
		{
			varType = QObject::tr("Table");
			QStringList treeTexts = { field, varName, "", QString("(%1)").arg(varType) };
			pNode = q_check_ptr(new TreeWidgetItem(treeTexts));
			sash_assume(nullptr != pNode);
			if (pNode == nullptr)
				continue;

			depth = kMaxLuaTableDepth;
			if (luaTableToTreeWidgetItem(field, pNode, o.as<sol::table>(), depth, "global"))
			{
				pNode->setData(0, Qt::UserRole, QString("global"));
				pNode->setIcon(0, QIcon(":/image/icon_class.svg"));
				pNode->setText(2, QString("(%1)").arg(pNode->childCount()));
				if (g_sysConstVarName.contains(it))
					nodesSystem.append(pNode);
				else
					nodesCustom.append(pNode);
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
		else if (o.is<long long>())
		{
			varType = QObject::tr("Int");
			varValueStr = util::toQString(o.as<long long>());
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
			varValueStr = QString("0x%1").arg(util::toQString(reinterpret_cast<long long>(o.as<sol::function>().pointer()), 16));
			continue;
		}
		else if (o == sol::lua_nil)
		{
			varType = QObject::tr("Nil");
			varValueStr = "nil";
		}
		else
		{
			varType = hashSolLuaType.value(static_cast<long long>(o.get_type()), QObject::tr("Unknown"));
			varValueStr = QString("0x%1").arg(util::toQString(reinterpret_cast<long long>(o.pointer()), 16));
		}

		if (varType.isEmpty())
			continue;

		QStringList treeTexts = { field, varName, varValueStr, QString("(%1)").arg(varType) };
		pNode = q_check_ptr(new TreeWidgetItem(treeTexts));
		sash_assume(pNode != nullptr);
		if (pNode == nullptr)
			continue;

		pNode->setData(0, Qt::UserRole, QString("global"));
		pNode->setIcon(0, QIcon(":/image/icon_field.svg"));
		if (g_sysConstVarName.contains(it))
			nodesSystem.append(pNode);
		else
			nodesCustom.append(pNode);
	}

	widgetSystem->addTopLevelItems(nodesSystem);
	widgetCustom->addTopLevelItems(nodesCustom);

	widgetSystem->blockSignals(false);
	widgetCustom->blockSignals(false);
}