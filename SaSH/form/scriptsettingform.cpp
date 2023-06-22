#include "stdafx.h"
#include "scriptsettingform.h"

#include "injector.h"
#include "signaldispatcher.h"


#include "crypto.h"
#include <QSpinBox>

extern util::SafeHash<QString, util::SafeHash<int, break_marker_t>> break_markers;//用於標記自訂義中斷點(紅點)
extern util::SafeHash < QString, util::SafeHash<int, break_marker_t>> forward_markers;//用於標示當前執行中斷處(黃箭頭)
extern util::SafeHash < QString, util::SafeHash<int, break_marker_t>> error_markers;//用於標示錯誤發生行(紅線)
extern util::SafeHash < QString, util::SafeHash<int, break_marker_t>> step_markers;//隱式標記中斷點用於單步執行(無)


ScriptSettingForm::ScriptSettingForm(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	installEventFilter(this);
	setAttribute(Qt::WA_DeleteOnClose);

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
	//排序
	ui.treeWidget_functionList->sortItems(0, Qt::AscendingOrder);


	ui.menuBar->setMinimumWidth(200);

	//載入固定狀態
	m_staticLabel.setParent(this);
	m_staticLabel.setFrameStyle(QFrame::NoFrame);
	m_staticLabel.setText(QString(tr("row:%1 | size:%2 | index:%3 | %4").arg(1).arg(0).arg(1).arg("CRLF")));
	m_staticLabel.setOpenExternalLinks(true);
	m_staticLabel.setStyleSheet("color: rgb(250, 250, 250);background-color: rgb(31, 31, 31);border:none");
	ui.statusBar->addPermanentWidget(&m_staticLabel);
	ui.statusBar->setStyleSheet("color: rgb(250, 250, 250);background-color: rgb(31, 31, 31);border:none");


	//ui.listView_log->setModel(thread->getScriptLogModel());

	ui.listView_log->setWordWrap(false);
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


	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &ScriptSettingForm::onApplyHashSettingsToUI, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::addForwardMarker, this, &ScriptSettingForm::onAddForwardMarker, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::addErrorMarker, this, &ScriptSettingForm::onAddErrorMarker, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::addBreakMarker, this, &ScriptSettingForm::onAddBreakMarker, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::addStepMarker, this, &ScriptSettingForm::onAddStepMarker, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::loadFileToTable, this, &ScriptSettingForm::loadFile, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::reloadScriptList, this, &ScriptSettingForm::onReloadScriptList, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptLabelRowTextChanged2, this, &ScriptSettingForm::onScriptLabelRowTextChanged, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::globalVarInfoImport, this, &ScriptSettingForm::onGlobalVarInfoImport, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::localVarInfoImport, this, &ScriptSettingForm::onLocalVarInfoImport, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptFinished, this, &ScriptSettingForm::onScriptStopMode, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptStarted, this, &ScriptSettingForm::onScriptStartMode, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptPaused2, this, &ScriptSettingForm::onScriptPauseMode, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptPaused, this, &ScriptSettingForm::onScriptPauseMode, Qt::QueuedConnection);
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

	const QString fileName(qgetenv("JSON_PATH"));
	if (fileName.isEmpty())
		return;

	if (!QFile::exists(fileName))
		return;
	util::Config config(fileName);

	currentFileName_ = config.readString(objectName(), "LastModifyFile");
	if (!currentFileName_.isEmpty() && QFile::exists(currentFileName_))
	{
		emit signalDispatcher.loadFileToTable(currentFileName_);
	}

	QString ObjectName = ui.widget->objectName();
	int fontSize = config.readInt(objectName(), ObjectName, "FontSize");
	if (fontSize > 0)
	{
		QFont font = ui.widget->getOldFont();
		font.setPointSize(fontSize);
		ui.widget->setNewFont(font);
	}

	ObjectName = ui.listView_log->objectName();
	fontSize = config.readInt(objectName(), ObjectName, "FontSize");
	if (fontSize > 0)
	{
		QFont font = ui.listView_log->font();
		font.setPointSize(fontSize);
		ui.listView_log->setFont(font);
	}

	ObjectName = ui.textBrowser->objectName();
	fontSize = config.readInt(objectName(), ObjectName, "FontSize");
	if (fontSize > 0)
	{
		QFont font = ui.textBrowser->font();
		font.setPointSize(fontSize);
		ui.textBrowser->setFont(font);
	}

	//
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
	connect(pSpeedSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [](int value)
		{
			Injector& injector = Injector::getInstance();
			injector.setValueHash(util::kScriptSpeedValue, value);
		});

	ui.mainToolBar->addWidget(pSpeedSpinBox);
}

ScriptSettingForm::~ScriptSettingForm()
{

}

void ScriptSettingForm::showEvent(QShowEvent* e)
{
	setAttribute(Qt::WA_Mapped);
	QMainWindow::showEvent(e);
}

void ScriptSettingForm::closeEvent(QCloseEvent* e)
{
	util::FormSettingManager formSettingManager(this);
	formSettingManager.saveSettings();

	const QString fileName(qgetenv("JSON_PATH"));
	if (fileName.isEmpty())
		return;

	if (!QFile::exists(fileName))
		return;
	util::Config config(fileName);

	config.write(objectName(), "LastModifyFile", currentFileName_);


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

void ScriptSettingForm::onApplyHashSettingsToUI()
{
	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull() && injector.server->IS_ONLINE_FLAG)
	{
		QString title = currentFileName_;
		QString newTitle = QString("[%1] %2").arg(injector.server->pc.name).arg(title);
		setWindowTitle(newTitle);
	}

	if (!injector.scriptLogModel.isNull())
		ui.listView_log->setModel(injector.scriptLogModel.get());

	pSpeedSpinBox->setValue(injector.getValueHash(util::kScriptSpeedValue));
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

	createSpeedSpinBox();
	//禁止編輯
	ui.widget->setReadOnly(true);

	ui.mainToolBar->setUpdatesEnabled(true);
}

void ScriptSettingForm::setMark(CodeEditor::SymbolHandler element, util::SafeHash<QString, util::SafeHash<int, break_marker_t>>& hash, int liner, bool b)
{
	do
	{
		if (liner == -1)
		{
			hash.clear();
			ui.widget->markerDeleteAll(element);
			break;
		}

		if (b)
		{
			break_marker_t bk = {};
			bk.line = liner;
			bk.content = ui.widget->text(liner);
			bk.count = NULL;
			bk.maker = static_cast<int>(element);
			hash[currentFileName_].insert(liner, bk);

			ui.widget->markerAdd(liner, element); // 添加标签
			onEditorCursorPositionChanged(liner, 0);
		}
		else if (!b)
		{
			util::SafeHash<int, break_marker_t> markers = hash.value(currentFileName_);
			if (markers.contains(liner))
				hash[currentFileName_].remove(liner);
			ui.widget->markerDelete(liner, element);
		}
	} while (false);
}

void ScriptSettingForm::onAddForwardMarker(int liner, bool b)
{
	setMark(CodeEditor::SymbolHandler::SYM_ARROW, forward_markers, liner, b);
}

void ScriptSettingForm::onAddErrorMarker(int liner, bool b)
{
	setMark(CodeEditor::SymbolHandler::SYM_TRIANGLE, error_markers, liner, b);
}

void ScriptSettingForm::onAddStepMarker(int liner, bool b)
{
	if (!b)
	{
		this->setMark(CodeEditor::SymbolHandler::SYM_STEP, step_markers, -1, false);
	}
	else
	{
		int size = ui.widget->lines();
		for (int i = 0; i < size; ++i)
		{
			this->setMark(CodeEditor::SymbolHandler::SYM_STEP, step_markers, i, true);
		}
		onScriptBreakMode();
	}
}

void ScriptSettingForm::reshowBreakMarker()
{
	const util::SafeHash<QString, util::SafeHash<int, break_marker_t>> mks = break_markers;
	for (auto it = mks.begin(); it != mks.end(); ++it)
	{
		QString fileName = it.key();
		if (fileName != currentFileName_)
			continue;

		const util::SafeHash<int, break_marker_t> mk = mks.value(fileName);
		for (const break_marker_t& it : mk)
		{
			ui.widget->markerAdd(it.line, CodeEditor::SymbolHandler::SYM_POINT);
		}
	}
	onBreakMarkInfoImport();
}

void ScriptSettingForm::onAddBreakMarker(int liner, bool b)
{
	do
	{
		if (liner == -1)
		{
			ui.widget->markerDeleteAll(CodeEditor::SymbolHandler::SYM_POINT);
			break;
		}

		if (b)
		{
			break_marker_t bk = {};
			bk.line = liner;
			bk.content = ui.widget->text(liner);
			if (bk.content.simplified().isEmpty() || bk.content.simplified().indexOf("//") == 0)
				return;
			bk.count = NULL;
			bk.maker = static_cast<int>(CodeEditor::SymbolHandler::SYM_POINT);
			break_markers[currentFileName_].insert(liner, bk);
			ui.widget->markerAdd(liner, CodeEditor::SymbolHandler::SYM_POINT);
		}
		else if (!b)
		{
			util::SafeHash<int, break_marker_t> markers = break_markers.value(currentFileName_);
			if (markers.contains(liner))
				break_markers[currentFileName_].remove(liner);

			ui.widget->markerDelete(liner, CodeEditor::SymbolHandler::SYM_POINT);
		}
	} while (false);
	onBreakMarkInfoImport();
}

void ScriptSettingForm::onBreakMarkInfoImport()
{

	QList<QTreeWidgetItem*> trees = {};
	ui.treeWidget_breakList->clear();
	ui.treeWidget_breakList->setColumnCount(4);
	ui.treeWidget_breakList->setHeaderLabels(QStringList{ tr("CONTENT"),tr("COUNT"), tr("ROW"), tr("FILE") });
	const util::SafeHash<QString, util::SafeHash<int, break_marker_t>> mks = break_markers;
	for (auto it = mks.begin(); it != mks.end(); ++it)
	{
		QString fileName = it.key();
		const util::SafeHash<int, break_marker_t> mk = mks.value(fileName);
		for (const break_marker_t& it : mk)
		{
			QTreeWidgetItem* item = q_check_ptr(new QTreeWidgetItem({ it.content, QString::number(it.count), QString::number(it.line + 1), fileName }));
			item->setIcon(0, QIcon("://image/icon_break.png"));
			trees.append(item);
		}
		ui.treeWidget_breakList->addTopLevelItems(trees);
	}
}

void ScriptSettingForm::on_widget_marginClicked(int margin, int line, Qt::KeyboardModifiers state)
{
	Q_UNUSED(margin);
	int mask = ui.widget->markersAtLine(line);

	switch (state)
	{
	case Qt::ControlModifier: // 按下Ctrl键
	case Qt::AltModifier: break;// 按下Alt键
	default:
	{
		if (ui.widget->isBreak(mask))
		{
			onAddBreakMarker(line, false);
		}
		else
		{
			onAddBreakMarker(line, true);
		}
		break;
	}
	}
}

static const QRegularExpression rexLoadComma(R"(,[ \t\f\v]{0,})");
static const QRegularExpression rexSetComma(R"(,)");
void ScriptSettingForm::fileSave(const QString& d, DWORD flag)
{
	Injector& injector = Injector::getInstance();
	if (injector.IS_SCRIPT_FLAG)
		return;

	const QString directoryName(QApplication::applicationDirPath() + "/script");
	const QDir dir(directoryName);
	if (!dir.exists())
		dir.mkdir(directoryName);

	const QString fileName(currentFileName_);//當前路徑

	if (fileName.isEmpty())
		return;

	//backup
	const QString backupName(fileName + ".bak");
	QFile::remove(backupName);
	QFile::copy(fileName, backupName);

	//紀錄光標位置
	int line = NULL, index = NULL;
	ui.widget->getCursorPosition(&line, &index);

	//紀錄選擇範圍
	int lineFrom = NULL, indexFrom = NULL, lineTo = NULL, indexTo = NULL;
	ui.widget->getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);

	//紀錄滾動條位置
	int v = ui.widget->verticalScrollBar()->value();


	QFile file(fileName);
	if (!file.open((QIODevice::OpenModeFlag)flag))
		return;

	QTextStream ts(&file);
	ts.setCodec(util::CODEPAGE_DEFAULT);
	ts.setGenerateByteOrderMark(true);
	ts.setLocale(QLocale::Chinese);

	QString content(d);
	//check if is not CRLF switch to CRLF

	content.replace("\r\n", "\n");
	content.replace("\n", "\r\n");

	QStringList contents = content.split("\r\n");
	//QStringList newcontents;
	QStringList newContents;
	int indentLevel = 0;
	for (const QString& line : contents)
	{
		QString trimmedLine = line.trimmed();
		if (trimmedLine.startsWith("end"))
		{
			indentLevel--;
		}
		QString indentedLine = QString("    ").repeated(indentLevel) + trimmedLine;
		newContents.append(indentedLine);
		if (trimmedLine.startsWith("function"))
		{
			indentLevel++;
		}
	}
	//int indent_begin = -1;
	//int indent_end = -1;
	//for (int i = 0; i < contents.size(); ++i)
	//{
	//	contents[i].replace(rexLoadComma, ",");
	//	contents[i].replace(rexSetComma, ", ");
	//	contents[i] = contents[i].trimmed();
	//	if (contents[i].startsWith("function") && indent_begin == -1)
	//	{
	//		indent_begin = i + 1;
	//	}
	//	else if (contents[i].startsWith("function") && indent_begin != -1)
	//	{
	//		indent_begin = -1;
	//	}
	//	else if ((contents[i].startsWith("end")) && indent_begin != -1 && indent_end == -1)
	//	{
	//		indent_end = i - 1;
	//		for (int j = indent_begin; j <= indent_end; ++j)
	//		{
	//			contents[j] = "    " + contents[j];
	//		}
	//		indent_begin = -1;
	//		indent_end = -1;
	//	}
	//}
	ts << newContents.join("\n");
	ts.flush();
	file.flush();
	file.close();

	ui.widget->selectAll();
	ui.widget->replaceSelectedText(contents.join("\r\n"));
	//回復選擇範圍
	//ui.widget->setSelection(lineFrom, indexFrom, lineTo, indexTo);
	//

	////回復光標位置
	ui.widget->setCursorPosition(line, index);
	ui.widget->ensureLineVisible(line);

	//回復滾動條位置
	ui.widget->verticalScrollBar()->setValue(v);

	ui.statusBar->showMessage(QString(tr("Script %1 saved")).arg(fileName), 3000);

	onWidgetModificationChanged(true);

	onAddErrorMarker(-1, false);
	onAddForwardMarker(-1, false);
	onAddStepMarker(-1, false);
	forward_markers.clear();
	error_markers.clear();
	step_markers.clear();
	reshowBreakMarker();
}

void ScriptSettingForm::onScriptTreeWidgetHeaderClicked(int logicalIndex)
{
	if (logicalIndex == 0)
	{
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.reloadScriptList();
	}
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

		util::loadAllFileLists(item, QApplication::applicationDirPath() + "/script/", &newScriptList);
		//compare with oldScriptList
		if (m_scriptList == newScriptList)
		{
			delete item;
			item = nullptr;
			break;
		}

		m_scriptList = newScriptList;
		ui.treeWidget_scriptList->setUpdatesEnabled(false);
		ui.treeWidget_scriptList->clear();
		ui.treeWidget_scriptList->addTopLevelItem(item);
		//展開全部第一層
		ui.treeWidget_scriptList->topLevelItem(0)->setExpanded(true);
		for (int i = 0; i < item->childCount(); i++)
		{
			ui.treeWidget_scriptList->expandItem(item->child(i));
		}

		ui.treeWidget_scriptList->sortItems(0, Qt::AscendingOrder);

		ui.treeWidget_scriptList->setUpdatesEnabled(true);
	} while (false);
}

void ScriptSettingForm::loadFile(const QString& fileName)
{
	util::QScopedFile f(fileName, QIODevice::ReadOnly | QIODevice::Text);
	if (!f.isOpen())
		return;

	QTextStream in(&f);
	in.setCodec(util::CODEPAGE_DEFAULT);
	QString c = in.readAll();
	c.replace("\r\n", "\n");


	currentFileName_ = fileName;
	ui.widget->setUpdatesEnabled(false);

	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull() && injector.server->IS_ONLINE_FLAG)
		setWindowTitle(QString("[%1] %2").arg(injector.server->pc.name).arg(currentFileName_));
	ui.widget->convertEols(QsciScintilla::EolWindows);
	ui.widget->setUtf8(true);
	ui.widget->setModified(false);
	ui.widget->selectAll();
	ui.widget->replaceSelectedText(c);
	ui.widget->setUpdatesEnabled(true);

	onAddErrorMarker(-1, false);
	onAddForwardMarker(-1, false);
	onAddStepMarker(-1, false);
	forward_markers.clear();
	error_markers.clear();
	step_markers.clear();

	reshowBreakMarker();
}

void ScriptSettingForm::onContinue()
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	emit signalDispatcher.scriptPaused2();
	onAddErrorMarker(-1, false);
	onAddForwardMarker(-1, false);
	onAddStepMarker(-1, false);
	forward_markers.clear();
	error_markers.clear();
	step_markers.clear();

	onScriptStartMode();
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

		onAddErrorMarker(-1, false);
		onAddForwardMarker(-1, false);
		onAddStepMarker(-1, false);
		forward_markers.clear();
		error_markers.clear();
		step_markers.clear();

		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.loadFileToTable(strpath);

	} while (false);

	IS_LOADING = false;
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
	if (name == "actionSave")
	{
		fileSave(ui.widget->text(), QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
		emit signalDispatcher.loadFileToTable(currentFileName_);
		emit signalDispatcher.reloadScriptList();
	}
	else if (name == "actionStart")
	{
		Injector& injector = Injector::getInstance();
		if (step_markers.size() == 0 && !injector.IS_SCRIPT_FLAG && QFile::exists(currentFileName_))
		{
			emit signalDispatcher.scriptStarted();
		}
	}
	else if (name == "actionPause")
	{
		emit signalDispatcher.scriptPaused2();
	}
	else if (name == "actionStop")
	{
		emit signalDispatcher.scriptStoped();
	}
	else if (name == "actionContinue")
	{
		onContinue();
	}
	else if (name == "actionSaveAs")
	{
		const QString directoryName(QApplication::applicationDirPath() + "/script");
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
				ts.setCodec(util::CODEPAGE_DEFAULT);
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
		QDesktopServices::openUrl(QUrl::fromLocalFile(QApplication::applicationDirPath() + "/script"));
	}
	else if (name == "actionStep")
	{
		onAddErrorMarker(-1, false);
		onAddForwardMarker(-1, false);
		emit signalDispatcher.scriptPaused2();
	}
	else if (name == "actionMark")
	{
		int line = -1;
		int index = -1;
		ui.widget->getCursorPosition(&line, &index);
		on_widget_marginClicked(NULL, line, Qt::NoModifier);
	}
	else if (name == "actionLogback")
	{
		Injector& injector = Injector::getInstance();
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
		QString strpath("");
		TreeWidgetItem* item = nullptr;

		auto finditem = [this](const QString& str)
		{
			//search if there has item with same name then break
			do
			{
				TreeWidgetItem* item = reinterpret_cast<TreeWidgetItem*>(ui.treeWidget_scriptList->topLevelItem(0));
				if (!item)
					break;

				int count = item->childCount();
				for (int j = 0; j < count; j++)
				{
					TreeWidgetItem* chileitem = reinterpret_cast<TreeWidgetItem*>(item->child(j));
					if (!chileitem)
						continue;
					if (!chileitem->text(0).isEmpty() && chileitem->text(0).compare(str) == 0)
						return true;
				}

			} while (false);
			return false;
		};

		for (;;)
		{
			strpath = (QApplication::applicationDirPath() + QString("/script/Untitled-%1.txt").arg(num));
			if (!QFile::exists(strpath) && !finditem(QString("Untitled-%1.txt").arg(num)) && !finditem(QString("Untitled-%1.txt*").arg(num)))
			{
				item = q_check_ptr(new TreeWidgetItem(QStringList{ QString("Untitled-%1.txt*").arg(num) }));
				if (!item) break;

				item->setIcon(0, QIcon(QPixmap(":/image/icon_txt.png")));

				ui.treeWidget_scriptList->topLevelItem(0)->insertChild(0, item);
				ui.widget->convertEols(QsciScintilla::EolWindows);
				ui.widget->setUtf8(true);
				ui.widget->selectAll();
				ui.widget->replaceSelectedText("");
				setWindowTitle(QString("[%1] %2").arg(0).arg(strpath));
				currentFileName_ = strpath;
				m_scripts.insert(strpath, ui.widget->text());

				ui.treeWidget_scriptList->setCurrentItem(item);

				onAddErrorMarker(-1, false);
				onAddForwardMarker(-1, false);
				onAddStepMarker(-1, false);
				forward_markers.clear();
				error_markers.clear();
				step_markers.clear();
				break;
			}
			else
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
}

void ScriptSettingForm::onWidgetModificationChanged(bool changed)
{
	if (!changed) return;

	m_scripts.insert(currentFileName_, ui.widget->text());
}

void ScriptSettingForm::on_comboBox_labels_clicked()
{
	static const QRegularExpression rex_divLabel(u8R"(([標|標][記|記]|function|label)\s+\p{Han}+)");
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
			const QString str = match.captured(0).simplified();
			if (!str.isEmpty())
			{
				ui.comboBox_labels->addItem(QIcon(":/image/icon_label.png"), str.simplified(), QVariant(i));
			}
		}
	}
	ui.comboBox_labels->setCurrentIndex(cur);
	ui.comboBox_labels->blockSignals(false);
}

void ScriptSettingForm::on_widget_cursorPositionChanged(int line, int index)
{
	const QsciScintilla::EolMode mode = ui.widget->eolMode();
	const QString modeStr(mode == QsciScintilla::EolWindows ? "CRLF" : mode == QsciScintilla::EolUnix ? "LF" : "CR");
	//獲取當前行字
	const QString lineText = ui.widget->text(line);
	m_staticLabel.setText(QString(tr("row:%1 | size:%2 | index:%3 | %4").arg(line + 1).arg(lineText.size()).arg(index).arg(modeStr)));
}

void ScriptSettingForm::on_widget_textChanged()
{
	const QString text(ui.widget->text());
	if (m_scripts.value(currentFileName_, "") != text)
	{
		m_scripts.insert(currentFileName_, text);
		m_isModified = true;
		ui.widget->setModified(true);
		emit ui.widget->modificationChanged(true);
	}
	else
	{
		m_isModified = false;
		ui.widget->setModified(false);
	}
	int line = -1, index = -1;
	ui.widget->getCursorPosition(&line, &index);
	on_widget_cursorPositionChanged(line, index);
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

void ScriptSettingForm::onScriptLabelRowTextChanged(int line, int max, bool noSelect)
{
	if (line < 0)
		line = 0;

	ui.widget->setCursorPosition(line, ui.widget->SendScintilla(QsciScintilla::SCI_LINELENGTH, line) - 1);
	ui.widget->SendScintilla(QsciScintilla::SCI_SETSELECTIONMODE, QsciScintilla::SC_SEL_LINES);

	ui.widget->setCaretLineBackgroundColor(QColor(71, 71, 71));//光標所在行背景顏色

	//確保光標示在可視範圍內
	ui.widget->ensureLineVisible(line);
}

//切換光標和焦點所在行
void ScriptSettingForm::onEditorCursorPositionChanged(int line, int index)
{
	if (line < 0)
		line = 0;

	ui.widget->setCursorPosition(line, ui.widget->SendScintilla(QsciScintilla::SCI_LINELENGTH, line - 1) - 1);
	ui.widget->SendScintilla(QsciScintilla::SCI_SETSELECTIONMODE, QsciScintilla::SC_SEL_LINES);

	if (NULL == index)
		ui.widget->setCaretLineBackgroundColor(QColor(71, 71, 71));//光標所在行背景顏色
	else
		ui.widget->setCaretLineBackgroundColor(QColor(0xff, 0x80, 0x80));//光標所在行背景顏色

	//確保光標示在可視範圍內
	ui.widget->ensureLineVisible(line);
}

//標記列表點擊
void ScriptSettingForm::on_comboBox_labels_currentIndexChanged(int index)
{
	QVariant var = ui.comboBox_labels->currentData();
	if (var.isValid())
	{
		int line = var.toInt();
		onEditorCursorPositionChanged(line, NULL);
	}
}

void ScriptSettingForm::onCallStackInfoChanged(const QHash <int, QString>& var)
{
	stackInfoImport(ui.treeWidget_debuger_callstack, var);
}

void ScriptSettingForm::onJumpStackInfoChanged(const QHash <int, QString>& var)
{
	stackInfoImport(ui.treeWidget_debuger_jmpstack, var);
}

void ScriptSettingForm::stackInfoImport(QTreeWidget* tree, const QHash <int, QString>& d)
{
	if (tree == nullptr)
		return;

	tree->setUpdatesEnabled(false);

	tree->clear();
	tree->setColumnCount(2);
	tree->setHeaderLabels(QStringList() << tr("row") << tr("content"));

	if (!d.isEmpty())
	{
		QList<QTreeWidgetItem*> trees;
		int size = d.size();
		for (auto it = d.begin(); it != d.end(); ++it)
		{
			trees.append(q_check_ptr(new QTreeWidgetItem({ QString::number(it.key()), it.value() })));
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
	tree->setHeaderLabels(QStringList() << tr("name") << tr("value") << tr("type"));

	if (!d.isEmpty())
	{
		QMap<QString, QVariant> map;
		QList<QTreeWidgetItem*> trees;

		for (auto it = d.begin(); it != d.end(); ++it)
		{
			map.insert(it.key(), it.value());
		}

		for (auto it = map.begin(); it != map.end(); ++it)
		{
			QString varName = it.key();
			QString varType;
			QString varValue;
			QVariant var = it.value();
			switch (var.type())
			{
			case QVariant::Int:
			{
				varType = tr("Int");
				varValue = QString::number(var.toInt());
				break;
			}
			case QVariant::UInt:
			{
				varType = tr("UInt");
				varValue = QString::number(var.toUInt());
				break;
			}
			case QVariant::Double:
			{
				varType = tr("Double");
				varValue = QString::number(var.toDouble(), 'f', 8);
				break;
			}
			case QVariant::String:
			{
				varType = tr("String");
				varValue = var.toString();
				break;
			}
			case QVariant::Bool:
			{
				varType = tr("Bool");
				varValue = var.toBool();
				break;
			}
			case QVariant::LongLong:
			{
				varType = tr("LongLong");
				varValue = QString::number(var.toLongLong());
				break;
			}
			case QVariant::ULongLong:
			{
				varType = tr("ULongLong");
				varValue = QString::number(var.toULongLong());
				break;
			}
			default:
			{
				varType = tr("unknown");
				varValue = var.toString();
				break;
			}

			}
			trees.append(q_check_ptr(new QTreeWidgetItem({ varName, varValue, QString("(%1)").arg(varType) })));
		}
		tree->addTopLevelItems(trees);
	}
	tree->setUpdatesEnabled(true);
}

//全局變量列表
void ScriptSettingForm::onGlobalVarInfoImport(const QHash<QString, QVariant>& d)
{
	if (currentGlobalVarInfo_ == d)
		return;

	currentGlobalVarInfo_ = d;
	QStringList systemVarList = {
		//utf8
		"tick", "stick", "chname", "chfname", "chlv", "chhp", "chmp", "chdp", "stone", "px", "py",
			"floor", "frname", "date", "time", "earnstone", "expbuff", "dlgid"
	};

	QStringList gb2312List = {
		//gb2312
		"毫秒时间戳", "秒时间戳",

		"人物主名","人物副名","人物等级","人物耐久力","人物气力","人物DP",
		"石币",

		"东坐标","南坐标","地图编号","地图",
		"日期","时间",
		"对话框ID",
	};

	QStringList big5List = {
		"毫秒時間戳", "秒時間戳",

		"人物主名","人物副名","人物等級","人物耐久力","人物氣力","人物DP",
		"石幣",

		"東坐標","南坐標","地圖編號","地圖",
		"日期","時間",
		"對話框ID",
	};

	QHash<QString, QVariant> globalVarInfo;
	QHash<QString, QVariant> customVarInfo;
	//如果key存在於系統變量列表中則添加到系統變量列表中

	UINT acp = GetACP();
	for (auto it = d.begin(); it != d.end(); ++it)
	{
		if (systemVarList.contains(it.key().simplified()))
		{
			globalVarInfo.insert(it.key(), it.value());
		}
		else if (acp == 936 && gb2312List.contains(it.key().simplified()))
		{
			globalVarInfo.insert(it.key(), it.value());
		}
		else if (acp != 936 && big5List.contains(it.key().simplified()))
		{
			globalVarInfo.insert(it.key(), it.value());
		}
		else if (!systemVarList.contains(it.key().simplified()) && !gb2312List.contains(it.key().simplified()) && !big5List.contains(it.key().simplified()))
		{
			customVarInfo.insert(it.key(), it.value());
		}
	}

	varInfoImport(ui.treeWidget_debuger_global, globalVarInfo);
	varInfoImport(ui.treeWidget_debuger_custom, customVarInfo);
}

//區域變量列表
void ScriptSettingForm::onLocalVarInfoImport(const QHash<QString, QVariant>& d)
{
	if (currentLocalVarInfo_ == d)
		return;

	currentLocalVarInfo_ = d;
	varInfoImport(ui.treeWidget_debuger_local, d);
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

	ui.widget->insert(str + " ");

}

void searchFiles(const QString& dir, const QString& fileNamePart, const QString& suffixWithDot, QList<QString>* result)
{
	QDir d(dir);
	if (!d.exists())
		return;

	QFileInfoList list = d.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
	for (const QFileInfo& fileInfo : list)
	{
		if (fileInfo.isFile())
		{
			if (fileInfo.fileName().contains(fileNamePart, Qt::CaseInsensitive) && fileInfo.suffix() == suffixWithDot.mid(1))
			{
				QFile file(fileInfo.absoluteFilePath());
				if (file.open(QIODevice::ReadOnly | QIODevice::Text))
				{
					QTextStream in(&file);
					in.setCodec("UTF-8");
					//將文件名置於前方
					QString fileContent = QString("# %1\n---\n%2").arg(fileInfo.fileName()).arg(in.readAll());
					file.close();

					result->append(fileContent);
				}
			}
		}
		else if (fileInfo.isDir())
		{
			searchFiles(fileInfo.absoluteFilePath(), fileNamePart, suffixWithDot, result);
		}
	}
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

		QString mdFullPath = QString("%1/lib/doc").arg(QCoreApplication::applicationDirPath());
		QDir dir(mdFullPath);
		if (!dir.exists())
			break;

		QStringList result;
		//以此目錄為起點遍歷查找所有文件名包含 \'str\' 的 .MD 文件
		searchFiles(mdFullPath, QString(R"('%1')").arg(str), ".md", &result);
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

	strpath = QApplication::applicationDirPath() + "/script/" + strpath;
	strpath.replace("*", "");

	QFileInfo info(strpath);
	QString suffix = "." + info.suffix();
	if (suffix.isEmpty())
		strpath += util::SCRIPT_SUFFIX_DEFAULT;
	if (suffix != util::SCRIPT_PRIVATE_SUFFIX_DEFAULT || suffix != util::SCRIPT_SUFFIX_DEFAULT)
		strpath.replace(suffix, util::SCRIPT_SUFFIX_DEFAULT);

	return strpath;
};

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
	//ui.treeWidget_scriptList->openPersistentEditor(item, 0);
	ui.treeWidget_scriptList->editItem(item, 0);

	//註冊檢測如果用戶班級完畢則關閉編輯狀態
	connect(ui.treeWidget_scriptList, &QTreeWidget::itemChanged, this, [this, currentPath, currentText](QTreeWidgetItem* newitem, int column)
		{
			if (column != 0 || !newitem)
				return;

			QString newtext = newitem->text(0);
			if (newtext.isEmpty())
			{
				newitem->setText(0, currentText);
				return;
			}

			if (newtext == currentText)
				return;

			QString str = getFullPath(newitem);
			if (str.isEmpty())
				return;

			if (str == currentPath)
				return;

			QFile file(currentPath);
			if (file.exists() && !QFile::exists(str))
			{
				file.rename(str);
			}

			if (!newtext.endsWith(util::SCRIPT_PRIVATE_SUFFIX_DEFAULT) && !newtext.endsWith(util::SCRIPT_SUFFIX_DEFAULT))
			{
				newtext += util::SCRIPT_SUFFIX_DEFAULT;
				newitem->setText(0, newtext);
			}
		});

}

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
			//ui.treeWidget_scriptList->openPersistentEditor(item, 0);
			ui.treeWidget_scriptList->editItem(item, 0);

			//註冊檢測如果用戶班級完畢則關閉編輯狀態
			connect(ui.treeWidget_scriptList, &QTreeWidget::itemChanged, this, [this, currentPath, currentText](QTreeWidgetItem* newitem, int column)
				{
					if (column != 0 || !newitem)
						return;

					QString newtext = newitem->text(0);
					if (newtext.isEmpty())
					{
						newitem->setText(0, currentText);
						return;
					}

					if (newtext == currentText)
						return;

					QString str = getFullPath(newitem);
					if (str.isEmpty())
						return;

					if (str == currentPath)
						return;

					QFile file(currentPath);
					if (file.exists() && !QFile::exists(str))
					{
						file.rename(str);
					}

					if (!newtext.endsWith(util::SCRIPT_PRIVATE_SUFFIX_DEFAULT) && !newtext.endsWith(util::SCRIPT_SUFFIX_DEFAULT))
					{
						newtext += util::SCRIPT_SUFFIX_DEFAULT;
						newitem->setText(0, newtext);
					}
				});
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
				onReloadScriptList();
			}
		});
}

void ScriptSettingForm::on_treeWidget_breakList_itemDoubleClicked(QTreeWidgetItem* item, int column)
{
	Q_UNUSED(column);
	if (item == nullptr)
		return;

	if (item->text(2).isEmpty()) return;

	if (!item->text(3).isEmpty() && item->text(3) == currentFileName_)
	{
		int line = item->text(2).toInt();
		ui.widget->setCursorPosition(line, 0);
		QString text = ui.widget->text(line);
		ui.widget->setSelection(line, 0, line, text.length());
		ui.widget->ensureLineVisible(line);
	}
}


void ScriptSettingForm::onEncryptSave()
{
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

	if (crypto.encodeScript(currentFileName_, password))
	{
		QString newFileName = currentFileName_;
		newFileName.replace(util::SCRIPT_SUFFIX_DEFAULT, util::SCRIPT_PRIVATE_SUFFIX_DEFAULT);
		currentFileName_ = newFileName;
		ui.statusBar->showMessage(QString(tr("Encrypt script %1 saved")).arg(newFileName), 3000);
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.loadFileToTable(newFileName);
	}
	else
	{
		ui.statusBar->showMessage(tr("Encrypt script save failed"), 3000);
	}
}

void ScriptSettingForm::onDecryptSave()
{
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
	if (crypto.decodeScript(currentFileName_, content))
	{
		QString newFileName = currentFileName_;
		newFileName.replace(util::SCRIPT_PRIVATE_SUFFIX_DEFAULT, util::SCRIPT_SUFFIX_DEFAULT);
		currentFileName_ = newFileName;
		ui.statusBar->showMessage(QString(tr("Decrypt script %1 saved")).arg(newFileName), 3000);
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
		emit signalDispatcher.loadFileToTable(newFileName);
	}
	else
	{
		ui.statusBar->showMessage(tr("Decrypt password is incorrect"), 3000);
	}
}