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

#include <QWidget>
#include "ui_luascriptform.h"

class CLua;
class LuaScriptForm : public QWidget
{
	Q_OBJECT

public:
	LuaScriptForm(QWidget* parent = nullptr);
	~LuaScriptForm();
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
	void onScriptResumed();
	void onScriptStoped();
	void onScriptFinished();
	void onScriptStarted();

	void onSpeedChanged(int value);

	void onApplyHashSettingsToUI();
private:
	void setTableWidgetItem(int row, int col, const QString& text);
	void resizeTableWidgetRow(int max);

private:
	Ui::LuaScriptFormClass ui;


	bool IS_LOADING = false;
	QStringList scriptList_;
	int selectedRow_ = 0;

	QScopedPointer<CLua> clua_;
	QThread* thread_ = nullptr;
};
