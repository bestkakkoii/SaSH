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

	void loadFile(const QString& fileName);

private slots:
	void onButtonClicked();

	void onReloadScriptList();
	void onScriptTreeWidgetHeaderClicked(int logicalIndex);
	void onScriptTreeWidgetDoubleClicked(QTreeWidgetItem* item, int column);


	void onScriptContentChanged(const QString& fileName, const QVariant& tokens);
	void onScriptTableWidgetClicked(QTableWidgetItem* item);
	void onScriptLabelRowTextChanged(int row, int max, bool noSelect);
	void onCurrentTableWidgetItemChanged(QTableWidgetItem* current, QTableWidgetItem* previous);

	void onScriptPaused();
	void onScriptFinished();
	void onStartScript();

private:
	void setTableWidgetItem(int row, int col, const QString& text);
	void resizeTableWidgetRow(int max);


private:
	Ui::ScriptFormClass ui;

	bool IS_LOADING = false;
	QStringList scriptList_;
	QString currentFileName_;
	int selectedRow_ = 0;

	QScopedPointer<Interpreter> interpreter_;

};
