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

#pragma once

#include <QMainWindow>
#include "ui_scriptsettingform.h"
#include <indexer.h>
#include "util.h"
#include "script/interpreter.h"


class QTextDocument;
class QSpinBox;
class ScriptSettingForm : public QMainWindow, public Indexer
{
	Q_OBJECT

public:
	explicit ScriptSettingForm(qint64 index, QWidget* parent = nullptr);

	virtual ~ScriptSettingForm();

protected:
	virtual void showEvent(QShowEvent* e) override;

	virtual void closeEvent(QCloseEvent* e) override;

private:
	void fileSave(const QString& d, DWORD flag);

	void replaceCommas(QString& inputList);

	QString formatCode(QString content);

	void onReloadScriptList();

	void setStepMarks();

	void setMark(CodeEditor::SymbolHandler element, util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>>& hash, qint64 liner, bool b);

	void varInfoImport(QTreeWidget* tree, const QHash<QString, QVariant>& d);

	void stackInfoImport(QTreeWidget* tree, const QVector<QPair<qint64, QString>>& vec);

	void reshowBreakMarker();

	void createSpeedSpinBox();

	void createScriptListContextMenu();

	QString getFullPath(QTreeWidgetItem* item);

	void setContinue();

	void createTreeWidgetItems(Parser* pparser, QList<QTreeWidgetItem*>* pTrees, const QHash<QString, QVariant>& d);

	void initStaticLabel();

signals:
	void editorCursorPositionChanged(int line, int index);

	void breakMarkInfoImport();

private slots:
	void onApplyHashSettingsToUI();
	void onScriptTreeWidgetHeaderClicked(int logicalIndex);
	void onScriptTreeWidgetDoubleClicked(QTreeWidgetItem* item, int column);
	void onActionTriggered();
	void onWidgetModificationChanged(bool changed);
	void onEditorCursorPositionChanged(int line, int index);
	void onAddForwardMarker(qint64 liner, bool b);
	void onAddErrorMarker(qint64 liner, bool b);
	void onAddStepMarker(qint64 liner, bool b);
	void onAddBreakMarker(qint64 liner, bool b);
	void onBreakMarkInfoImport();
	void onScriptTreeWidgetItemChanged(QTreeWidgetItem* newitem, int column);

	void onScriptStartMode();
	void onScriptStopMode();
	void onScriptBreakMode();
	void onScriptPauseMode();

	void onSpeedChanged(int value);

	void onScriptLabelRowTextChanged(int row, int max, bool noSelect);

	void loadFile(const QString& fileName);
	void onVarInfoImport(void* p, const QVariantHash&);

	void onSetStaticLabelLineText(int line, int index);

	void onCallStackInfoChanged(const QVariant& var);
	void onJumpStackInfoChanged(const QVariant& var);

	void onEncryptSave();
	void onDecryptSave();

	void on_comboBox_labels_clicked();
	void on_comboBox_functions_clicked();
	void on_comboBox_labels_currentIndexChanged(int index);
	void on_comboBox_functions_currentIndexChanged(int);
	void on_widget_cursorPositionChanged(int line, int index);
	void on_widget_textChanged();
	void on_lineEdit_searchFunction_textChanged(const QString& text);
	void on_widget_marginClicked(int margin, int line, Qt::KeyboardModifiers state);
	void on_treeWidget_functionList_itemDoubleClicked(QTreeWidgetItem* item, int column);
	void on_treeWidget_functionList_itemSelectionChanged();
	void on_treeWidget_functionList_itemClicked(QTreeWidgetItem* item, int column);
	void on_treeWidget_scriptList_itemClicked(QTreeWidgetItem* item, int column);
	void on_treeWidget_breakList_itemDoubleClicked(QTreeWidgetItem* item, int column);

	void on_listView_log_doubleClicked(const QModelIndex& index);

private:
	Ui::ScriptSettingFormClass ui;

	FILETIME idleTime_ = { 0, 0 };
	FILETIME kernelTime_ = { 0, 0 };
	FILETIME userTime_ = { 0, 0 };

	FastLabel* usageLabel_ = nullptr;
	FastLabel* lineLable_ = nullptr;
	FastLabel* sizeLabel_ = nullptr;
	FastLabel* indexLabel_ = nullptr;
	FastLabel* eolLabel_ = nullptr;

	QTimer* usageTimer_ = nullptr;
	bool IS_LOADING = false;
	bool isModified_ = false;
	QStringList scriptList_;

	QHash<QString, QString> scripts_;
	QHash<QString, QVariant> currentGlobalVarInfo_;
	QHash<QString, QVariant> currentLocalVarInfo_;
	QHash<QString, QSharedPointer<QTextDocument>> document_;
	QSpinBox* pSpeedSpinBox = nullptr;

	QString currentRenameText_ = "";
	QString currentRenamePath_ = "";
};