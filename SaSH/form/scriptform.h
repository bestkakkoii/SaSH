﻿/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2024 All Rights Reserved.
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
#include "ui_scriptform.h"
#include <QTreeWidgetItem>
#include <QTableWidgetItem>
#include <QThread>
#include <indexer.h>

struct Token;
class Interpreter;
class ScriptForm : public QWidget, public Indexer
{
	Q_OBJECT

public:
	ScriptForm(long long index, QWidget* parent);

	virtual ~ScriptForm();

private slots:
	void loadFile(const QString& fileName, bool start = false);

	void onButtonClicked();

	void onReloadScriptList();

	void onScriptTreeWidgetHeaderClicked(int logicalIndex);

	void onScriptTreeWidgetDoubleClicked(QTreeWidgetItem* item, int column);

	void onScriptContentChanged(const QString& fileName, const QVariant& tokens);

	void onScriptLabelRowTextChanged(long long row, long long max, bool noSelect);

	void onCurrentTableWidgetItemChanged(QTableWidgetItem* current, QTableWidgetItem* previous);

	void onScriptPaused();

	void onScriptResumed();

	void onScriptStoped();

	void onScriptFinished();

	void onScriptStarted();

	void onSpeedChanged(int value);

	void onApplyHashSettingsToUI();

protected:
	virtual void showEvent(QShowEvent* e) override
	{
		setAttribute(Qt::WA_Mapped);
		QWidget::showEvent(e);
	}

private:
	void resizeTableWidgetRow(long long max);

private:
	Ui::ScriptFormClass ui;

	QStringList scriptList_;

	int selectedRow_ = 0;

	std::unique_ptr<Interpreter> interpreter_;

};
