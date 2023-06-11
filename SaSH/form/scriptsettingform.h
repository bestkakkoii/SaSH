#pragma once

#include <QMainWindow>
#include "ui_scriptsettingform.h"

class ScriptSettingForm : public QMainWindow
{
	Q_OBJECT

public:
	ScriptSettingForm(QWidget* parent = nullptr);
	~ScriptSettingForm();
protected:
	void showEvent(QShowEvent* e) override;
	void closeEvent(QCloseEvent* e) override;
	bool eventFilter(QObject* obj, QEvent* e) override;

private:
	void fileSave(const QString& d, DWORD flag);

	void onReloadScriptList();

	void setMark(CodeEditor::SymbolHandler element, int liner, bool b);

	void varInfoImport(QTreeWidget* tree, const QHash<QString, QVariant>& d);

private slots:
	void onApplyHashSettingsToUI();
	void onScriptTreeWidgetHeaderClicked(int logicalIndex);
	void onScriptTreeWidgetDoubleClicked(QTreeWidgetItem* item, int column);
	void onActionTriggered();
	void onWidgetModificationChanged(bool changed);
	void on_comboBox_labels_clicked();
	void on_comboBox_labels_currentIndexChanged(int index);
	void onEditorCursorPositionChanged(int line, int index);
	void on_widget_cursorPositionChanged(int line, int index);
	void on_widget_textChanged();
	void on_lineEdit_searchFunction_textChanged(const QString& text);
	void onAddErrorMarker(int liner, bool b);
	void onScriptLabelRowTextChanged(int row, int max, bool noSelect);
	void on_treeWidget_functionList_itemDoubleClicked(QTreeWidgetItem* item, int column);
	void on_treeWidget_functionList_itemClicked(QTreeWidgetItem* item, int column);
	void loadFile(const QString& fileName);
	void onGlobalVarInfoImport(const QHash<QString, QVariant>& d);
	void onLocalVarInfoImport(const QHash<QString, QVariant>& d);
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
};
