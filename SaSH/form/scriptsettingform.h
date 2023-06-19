#pragma once

#include <QMainWindow>
#include "ui_scriptsettingform.h"
#include "util.h"
#include "script/interpreter.h"

class QSpinBox;
class ScriptSettingForm : public QMainWindow
{
	Q_OBJECT

public:
	ScriptSettingForm(QWidget* parent = nullptr);
	virtual ~ScriptSettingForm();
protected:
	void showEvent(QShowEvent* e) override;
	void closeEvent(QCloseEvent* e) override;
	bool eventFilter(QObject* obj, QEvent* e) override;

private:
	void fileSave(const QString& d, DWORD flag);

	void onReloadScriptList();

	void setMark(CodeEditor::SymbolHandler element, util::SafeHash<QString, util::SafeHash<int, break_marker_t>>& hash, int liner, bool b);

	void varInfoImport(QTreeWidget* tree, const QHash<QString, QVariant>& d);

	void reshowBreakMarker();

	void createSpeedSpinBox();

	void createScriptListContextMenu();

	QString getFullPath(QTreeWidgetItem* item);

private slots:
	void onApplyHashSettingsToUI();
	void onScriptTreeWidgetHeaderClicked(int logicalIndex);
	void onScriptTreeWidgetDoubleClicked(QTreeWidgetItem* item, int column);
	void onActionTriggered();
	void onWidgetModificationChanged(bool changed);
	void onEditorCursorPositionChanged(int line, int index);
	void onAddForwardMarker(int liner, bool b);
	void onAddErrorMarker(int liner, bool b);
	void onAddStepMarker(int liner, bool b);
	void onAddBreakMarker(int liner, bool b);
	void onBreakMarkInfoImport();

	void onScriptStartMode();
	void onScriptStopMode();
	void onScriptBreakMode();
	void onScriptPauseMode();

	void onScriptLabelRowTextChanged(int row, int max, bool noSelect);

	void loadFile(const QString& fileName);
	void onGlobalVarInfoImport(const QHash<QString, QVariant>& d);
	void onLocalVarInfoImport(const QHash<QString, QVariant>& d);
	void onContinue();

	void onEncryptSave();
	void onDecryptSave();

	void on_comboBox_labels_clicked();
	void on_comboBox_labels_currentIndexChanged(int index);
	void on_widget_cursorPositionChanged(int line, int index);
	void on_widget_textChanged();
	void on_lineEdit_searchFunction_textChanged(const QString& text);
	void on_widget_marginClicked(int margin, int line, Qt::KeyboardModifiers state);
	void on_treeWidget_functionList_itemDoubleClicked(QTreeWidgetItem* item, int column);
	void on_treeWidget_functionList_itemClicked(QTreeWidgetItem* item, int column);
	void on_treeWidget_scriptList_itemClicked(QTreeWidgetItem* item, int column);
private:
	Ui::ScriptSettingFormClass ui;
	QLabel m_staticLabel;
	bool IS_LOADING = false;
	bool m_isModified = false;
	QStringList m_scriptList;
	QString currentFileName_;
	QHash<QString, QString> m_scripts;
	QHash<QString, QVariant> currentGlobalVarInfo_;
	QHash<QString, QVariant> currentLocalVarInfo_;

	QSpinBox* pSpeedSpinBox = nullptr;
};
