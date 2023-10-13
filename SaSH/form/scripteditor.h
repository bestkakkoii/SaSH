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
#include "ui_scripteditor.h"
#include <indexer.h>
#include "util.h"
#include "script/interpreter.h"


class QTextDocument;
class QSpinBox;
class ScriptEditor : public QMainWindow, public Indexer
{
	Q_OBJECT

public:
	ScriptEditor(qint64 index, QWidget* parent);

	virtual ~ScriptEditor();

protected:
	virtual void showEvent(QShowEvent* e) override;

	virtual void closeEvent(QCloseEvent* e) override;

private:
	void fileSave(QString content);

	void replaceCommas(QString& inputList);

	QString formatCode(QString content);

	void onReloadScriptList();

	void setStepMarks();

	void setMark(CodeEditor::SymbolHandler element, util::SafeHash<QString, util::SafeHash<qint64, break_marker_t>>& hash, qint64 liner, bool b);

	void reshowBreakMarker();

	void createSpeedSpinBox();

	void createScriptListContextMenu();

	QString getFullPath(TreeWidgetItem* item);

	void setContinue();

	void createTreeWidgetItems(TreeWidget* widgetA, TreeWidget* widgetB, Parser* pparser, const QHash<QString, QVariant>& d, const QStringList& globalNames);

	void initStaticLabel();

	Q_INVOKABLE void init();

	Q_INVOKABLE void onNewFile();

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
	void onVarInfoImport(void* p, const QVariantHash&, const QStringList& globalNames);

	void onSetStaticLabelLineText(int line, int index);

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

	void on_mainToolBar_orientationChanged(Qt::Orientation orientation)
	{
		if (orientation == Qt::Horizontal)
		{
			ui.mainToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
			if (pSpeedSpinBox != nullptr)
			{
				pSpeedSpinBox->show();
				pSpeedSpinBox->setFixedWidth(spinRecordedWidth_);
			}
			if (pSpeedDescLabel_ != nullptr)
			{
				pSpeedDescLabel_->show();
				pSpeedDescLabel_->setFixedWidth(spinRecordedWidth_);
			}
		}
		else
		{
			ui.mainToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
			if (pSpeedSpinBox != nullptr)
			{
				pSpeedSpinBox->hide();
				spinRecordedWidth_ = pSpeedSpinBox->width();
				pSpeedSpinBox->setFixedWidth(18);
			}

			if (pSpeedDescLabel_ != nullptr)
			{
				pSpeedDescLabel_->hide();
				pSpeedDescLabel_->setFixedWidth(0);
			}
		}
	}

private:
	Ui::ScriptEditorClass ui;
	qint64 spinRecordedWidth_ = 0;

	FILETIME idleTime_ = { 0, 0 };
	FILETIME kernelTime_ = { 0, 0 };
	FILETIME userTime_ = { 0, 0 };

	FastLabel* usageLabel_ = nullptr;
	FastLabel* lineLable_ = nullptr;
	FastLabel* sizeLabel_ = nullptr;
	FastLabel* indexLabel_ = nullptr;
	FastLabel* eolLabel_ = nullptr;

	QTimer* usageTimer_ = nullptr;

	bool isModified_ = false;
	QStringList scriptList_;

	QHash<QString, QString> scripts_;
	QHash<QString, QVariant> currentGlobalVarInfo_;
	QHash<QString, QVariant> currentLocalVarInfo_;
	QHash<QString, QSharedPointer<QTextDocument>> document_;
	QLabel* pSpeedDescLabel_ = nullptr;
	QSpinBox* pSpeedSpinBox = nullptr;

	QString currentRenameText_ = "";
	QString currentRenamePath_ = "";
};