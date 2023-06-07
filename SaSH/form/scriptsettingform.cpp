#include "stdafx.h"
#include "scriptsettingform.h"

#include "util.h"
#include "injector.h"
#include "signaldispatcher.h"

ScriptSettingForm::ScriptSettingForm(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	installEventFilter(this);
	setAttribute(Qt::WA_DeleteOnClose);
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

	ui.dockWidget_debuger->hide();

	ui.treeWidget_scriptList->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui.treeWidget_scriptList->resizeColumnToContents(1);

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

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &ScriptSettingForm::onApplyHashSettingsToUI, Qt::UniqueConnection);

	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull())
		ui.listView_log->setModel(injector.server->scriptLogModel.get());

	util::FormSettingManager formSettingManager(this);
	formSettingManager.loadSettings();
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

	QMainWindow::closeEvent(e);
}

void ScriptSettingForm::onApplyHashSettingsToUI()
{
	Injector& injector = Injector::getInstance();
	if (!injector.server.isNull())
		ui.listView_log->setModel(injector.server->scriptLogModel.get());
}

static const QRegularExpression rexLoadComma(R"(,[ \t\f\v]{0,})");
static const QRegularExpression rexSetComma(R"(,)");
void ScriptSettingForm::fileSave(const QString& d, DWORD flag)
{
	const QString directoryName(QApplication::applicationDirPath() + "/script");
	const QDir dir(directoryName);
	if (!dir.exists())
		dir.mkdir(directoryName);

	const QString fileName("");//當前路徑

	if (fileName.isEmpty())
		return;

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
	for (QString& qline : contents)
	{
		QString trimmedStr = qline.simplified();
		if (trimmedStr.isEmpty())
		{
			qline = trimmedStr;
			//newcontents.append("");
			continue;
		}

		qline.replace(rexLoadComma, ",");
		qline.replace(rexSetComma, ", ");

		//紀錄左側空格
		int space = 0;
		int size = qline.size();
		int i = 0;
		for (i = 0; i < size; i++)
		{
			if (qline.at(i) == ' ')
				++space;
			else
				break;
		}

		qline = trimmedStr; //去除左右中空格
		//如果為空 則不處理
		if (qline.isEmpty()) continue;

		//補回space
		for (i = 0; i < space; i++)
			qline.insert(0, " ");

		//newcontents.append(line);
	}
	ts << contents.join("\n");
	ts.flush();
	file.flush();
	file.close();

	//ui.widget->clear();
	//ui.widget->setText(contents.join("\r\n"));
	ui.widget->selectAll();
	ui.widget->replaceSelectedText(contents.join("\r\n"));
	//回復選擇範圍
	ui.widget->setSelection(lineFrom, indexFrom, lineTo, indexTo);

	//回復光標位置
	ui.widget->setCursorPosition(line, index);

	//回復滾動條位置
	ui.widget->verticalScrollBar()->setValue(v);

	ui.statusBar->showMessage(QString(tr("Script %1 saved")).arg(fileName), 2000);

	//on_widget_modificationChanged(true);
}