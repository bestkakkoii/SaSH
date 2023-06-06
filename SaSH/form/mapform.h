#pragma once

#include <QWidget>
#include "ui_mapform.h"
class Interpreter;
class MapForm : public QWidget
{
	Q_OBJECT

public:
	MapForm(QWidget* parent = nullptr);
	virtual ~MapForm();

private slots:
	void onButtonClicked();

	void onUpdateNpcList(int floor);

	void onTableWidgetCellDoubleClicked(int row, int col);

	void onScriptFinished();
private:
	void updateTableWidgetContent(int row, int col, const QString& text);
	void resizeTableWidgetRow(int max);

private:
	Ui::MapFormClass ui;
	QHash<int, QPoint> npc_hash_;

	QScopedPointer<Interpreter> interpreter_;
};
