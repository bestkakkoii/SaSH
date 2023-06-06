#pragma once
#include <QWidget>
#include "ui_scriptform.h"
#include <QTreeWidgetItem>
#include <QTableWidgetItem>
#include <QThread>

struct Token;
class Interpreter;
class ScriptForm : public QWidget
{
	Q_OBJECT

public:
	ScriptForm(QWidget* parent = nullptr);
	~ScriptForm();

private slots:
	void onButtonClicked();

	void onReloadScriptList();
	void onScriptTreeWidgetHeaderClicked(int logicalIndex);
	void onScriptTreeWidgetDoubleClicked(QTreeWidgetItem* item, int column);

	void onScriptContentChanged(const QString& fileName);
	void onScriptTableWidgetClicked(QTableWidgetItem* item);
	void onScriptLabelRowTextChanged(int row, bool noSelect);
	void onCurrentTableWidgetItemChanged(QTableWidgetItem* current, QTableWidgetItem* previous);
	void onMenualRowChanged(int row, bool noSelect);

	void onScriptPaused();
	void onScriptFinished();
private:
	void setTableWidgetItem(int row, int col, const QString& text);
	void resizeTableWidgetRow(int max);
	int getCurrentMaxRow();
private:
	Ui::ScriptFormClass ui;

	bool IS_LOADING = false;
	QStringList scriptList_;
	QString currentFileName_;
	int selectedRow_ = 0;
	QHash<int, QMap<int, Token>> tokens_;//當前主腳本tokens
	QScopedPointer<Interpreter> interpreter_;

};
