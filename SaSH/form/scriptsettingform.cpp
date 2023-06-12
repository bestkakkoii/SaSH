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

	connect(ui.actionSave, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionSaveAs, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionDirectory, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionImport, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionAutoCleanLog, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionAutoFollow, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionNew, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionStart, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionPause, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);
	connect(ui.actionStop, &QAction::triggered, this, &ScriptSettingForm::onActionTriggered);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::applyHashSettingsToUI, this, &ScriptSettingForm::onApplyHashSettingsToUI, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::addErrorMarker, this, &ScriptSettingForm::onAddErrorMarker, Qt::UniqueConnection);
	connect(&signalDispatcher, &SignalDispatcher::loadFileToTable, this, &ScriptSettingForm::loadFile, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::reloadScriptList, this, &ScriptSettingForm::onReloadScriptList, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::scriptLabelRowTextChanged, this, &ScriptSettingForm::onScriptLabelRowTextChanged, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::globalVarInfoImport, this, &ScriptSettingForm::onGlobalVarInfoImport, Qt::QueuedConnection);
	connect(&signalDispatcher, &SignalDispatcher::localVarInfoImport, this, &ScriptSettingForm::onLocalVarInfoImport, Qt::QueuedConnection);
	Injector& injector = Injector::getInstance();
	if (!injector.scriptLogModel.isNull())
		ui.listView_log->setModel(injector.scriptLogModel.get());


	emit signalDispatcher.reloadScriptList();

	util::FormSettingManager formSettingManager(this);
	formSettingManager.loadSettings();

	//ui.webEngineView->setUrl(QUrl("https://gitee.com/Bestkakkoii/sash/wikis/pages"));
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
	if (!injector.scriptLogModel.isNull())
		ui.listView_log->setModel(injector.scriptLogModel.get());
}

void ScriptSettingForm::setMark(CodeEditor::SymbolHandler element, int liner, bool b)
{
	do
	{
		if (liner == -1)
		{
			ui.widget->markerDeleteAll(element);
			break;
		}

		if (b)
		{
			ui.widget->markerAdd(liner, element); // 添加标签
			onEditorCursorPositionChanged(liner, 0);
		}
		else if (!b)
		{
			ui.widget->markerDelete(liner, element);
		}
	} while (false);
}

void ScriptSettingForm::onAddErrorMarker(int liner, bool b)
{
	setMark(CodeEditor::SymbolHandler::SYM_TRIANGLE, liner, b);
}

static const QRegularExpression rexLoadComma(R"(,[ \t\f\v]{0,})");
static const QRegularExpression rexSetComma(R"(,)");
void ScriptSettingForm::fileSave(const QString& d, DWORD flag)
{
	const QString directoryName(QApplication::applicationDirPath() + "/script");
	const QDir dir(directoryName);
	if (!dir.exists())
		dir.mkdir(directoryName);

	const QString fileName(currentFileName_);//當前路徑

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
	int indent_begin = -1;
	int indent_end = -1;
	for (int i = 0; i < contents.size(); ++i)
	{
		contents[i].replace(rexLoadComma, ",");
		contents[i].replace(rexSetComma, ", ");
		contents[i] = contents[i].trimmed();
		if (contents[i].startsWith("標記") && indent_begin == -1)
		{
			indent_begin = i + 1;
		}
		else if (contents[i].startsWith("標記") && indent_begin != -1)
		{
			indent_begin = -1;
		}
		else if ((contents[i].startsWith("返回") || contents[i].startsWith("結束")) && indent_begin != -1 && indent_end == -1)
		{
			indent_end = i - 1;
			for (int j = indent_begin; j <= indent_end; ++j)
			{
				contents[j] = "    " + contents[j];
			}
			indent_begin = -1;
			indent_end = -1;
		}
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

	onWidgetModificationChanged(true);
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
		ui.treeWidget_scriptList->clear();
		ui.treeWidget_scriptList->addTopLevelItem(item);
		//展開全部第一層
		ui.treeWidget_scriptList->topLevelItem(0)->setExpanded(true);
		for (int i = 0; i < item->childCount(); i++)
		{
			ui.treeWidget_scriptList->expandItem(item->child(i));
		}

		ui.treeWidget_scriptList->sortItems(0, Qt::AscendingOrder);
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
	ui.widget->clear();
	setWindowTitle(QString("[%1] %2").arg(0).arg(currentFileName_));
	ui.widget->convertEols(QsciScintilla::EolWindows);
	ui.widget->setUtf8(true);
	ui.widget->setModified(false);
	ui.widget->setText(c);
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
		emit signalDispatcher.scriptStarted();
	}
	else if (name == "actionPause")
	{
		emit signalDispatcher.scriptPaused2();
	}
	else if (name == "actionStop")
	{
		emit signalDispatcher.scriptStoped();
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
		fd.setOption(QFileDialog::DontUseNativeDialog, true);
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
	else if (name == "actionImport")
	{

	}
	else if (name == "actionAutoCleanLog")
	{

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
				ui.widget->clear();
				ui.widget->convertEols(QsciScintilla::EolWindows);
				ui.widget->setUtf8(true);

				setWindowTitle(QString("[%1] %2").arg(0).arg(strpath));
				currentFileName_ = strpath;
				m_scripts.insert(strpath, ui.widget->text());

				ui.treeWidget_scriptList->setCurrentItem(item);
				break;
			}
			else
				++num;
		}
	}
}

void ScriptSettingForm::onWidgetModificationChanged(bool changed)
{
	if (!changed) return;

	m_scripts.insert(currentFileName_, ui.widget->text());
}

static const QRegularExpression rex_divLabel(u8R"([標|標][記|記]\s+\p{Han}+)");
void ScriptSettingForm::on_comboBox_labels_clicked()
{
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

void ScriptSettingForm::onScriptLabelRowTextChanged(int row, int max, bool noSelect)
{
	onEditorCursorPositionChanged(row, NULL);
}

//切換光標和焦點所在行
void ScriptSettingForm::onEditorCursorPositionChanged(int line, int index)
{
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

void ScriptSettingForm::varInfoImport(QTreeWidget* tree, const QHash<QString, QVariant>& d)
{
	if (tree == nullptr)
		return;

	QList<QTreeWidgetItem*> trees;
	tree->clear();
	tree->setColumnCount(3);
	tree->setHeaderLabels(QStringList() << tr("name") << tr("value") << tr("type"));
	for (auto it = d.begin(); it != d.end(); ++it)
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

//全局變量列表
void ScriptSettingForm::onGlobalVarInfoImport(const QHash<QString, QVariant>& d)
{
	if (currentGlobalVarInfo_ == d)
		return;

	currentGlobalVarInfo_ = d;
	varInfoImport(ui.treeWidget_debuger_global, d);
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

void ScriptSettingForm::on_treeWidget_functionList_itemClicked(QTreeWidgetItem* item, int column)
{

}