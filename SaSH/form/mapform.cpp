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

#include "stdafx.h"
#include "mapform.h"

#include "util.h"
#include "signaldispatcher.h"

#include "injector.h"
#include "script/interpreter.h"


MapForm::MapForm(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	qRegisterMetaType<QVariant>("QVariant");
	qRegisterMetaType<QVariant>("QVariant&");

	auto setTableWidget = [](QTableWidget* tableWidget)->void
	{
		//tablewidget set single selection
		tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
		//tablewidget set selection behavior
		tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
		//set auto resize to form size
		tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
		tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

		tableWidget->setStyleSheet(R"(
		QTableWidget { font-size:11px; } 
			QTableView::item:selected { background-color: black; color: white;
		})");
		tableWidget->verticalHeader()->setDefaultSectionSize(11);
		tableWidget->horizontalHeader()->setStretchLastSection(true);
		tableWidget->horizontalHeader()->setHighlightSections(false);
		tableWidget->verticalHeader()->setHighlightSections(false);
		tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
		tableWidget->verticalHeader()->setDefaultAlignment(Qt::AlignLeft);
		constexpr int max_row = 10;
		for (int row = 0; row < max_row; ++row)
		{
			tableWidget->insertRow(row);
		}

		int rowCount = tableWidget->rowCount();
		int columnCount = tableWidget->columnCount();
		for (int row = 0; row < rowCount; ++row)
		{
			for (int column = 0; column < columnCount; ++column)
			{
				QTableWidgetItem* item = new QTableWidgetItem("");
				if (item)
					tableWidget->setItem(row, column, item);
			}
		}
	};

	setTableWidget(ui.tableWidget_map);

	QList<QPushButton*> buttonList = util::findWidgets<QPushButton>(this);
	for (auto& button : buttonList)
	{
		if (button)
			connect(button, &QPushButton::clicked, this, &MapForm::onButtonClicked, Qt::UniqueConnection);
	}

	connect(ui.tableWidget_map, &QTableWidget::cellDoubleClicked, this, &MapForm::onTableWidgetCellDoubleClicked, Qt::UniqueConnection);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	connect(&signalDispatcher, &SignalDispatcher::updateNpcList, this, &MapForm::onUpdateNpcList, Qt::UniqueConnection);

}

MapForm::~MapForm()
{
	if (!interpreter_.isNull() && interpreter_->isRunning())
	{
		interpreter_->stop();
	}
}

void MapForm::onScriptFinished()
{
	ui.pushButton_findpath_stop->setEnabled(false);
	ui.pushButton_findpath_start->setEnabled(true);
}

void MapForm::onButtonClicked()
{
	QPushButton* pPushButton = qobject_cast<QPushButton*>(sender());
	if (!pPushButton)
		return;

	QString name = pPushButton->objectName();
	if (name.isEmpty())
		return;

	if (name == "pushButton_findpath_start")
	{
		Injector& injector = Injector::getInstance();
		if (injector.server.isNull())
			return;

		if (!injector.server->getOnlineFlag())
			return;

		if (!interpreter_.isNull() && interpreter_->isRunning())
			return;

		interpreter_.reset(new Interpreter());
		connect(interpreter_.data(), &Interpreter::finished, this, &MapForm::onScriptFinished, Qt::UniqueConnection);

		int x = ui.spinBox_findpath_x->value();
		int y = ui.spinBox_findpath_y->value();

		interpreter_->doString(QString(u8"findpath %1, %2, 1").arg(x).arg(y), nullptr, Interpreter::kNotShare);
		ui.pushButton_findpath_stop->setEnabled(true);
		ui.pushButton_findpath_start->setEnabled(false);
	}
	else if (name == "pushButton_findpath_stop")
	{
		if (interpreter_.isNull())
		{
			return;
		}

		if (!interpreter_->isRunning())
			return;

		interpreter_->stop();
		ui.pushButton_findpath_stop->setEnabled(false);
		ui.pushButton_findpath_start->setEnabled(true);
	}
}

void MapForm::updateTableWidgetContent(int row, int col, const QString& text)
{
	QTableWidgetItem* item = ui.tableWidget_map->item(row, col);
	if (item)
	{
		item->setText(text);
		item->setToolTip(text);
	}
	else
	{
		item = new QTableWidgetItem(text);
		if (item)
		{
			item->setText(text);
			item->setToolTip(text);
			ui.tableWidget_map->setItem(row, col, item);
		}
	}
}

void MapForm::resizeTableWidgetRow(int max)
{
	//set row count
	ui.tableWidget_map->setRowCount(max);
	int current = ui.tableWidget_map->rowCount();
	if (current > max)
	{
		//insert till max
		for (int i = current; i < max; ++i)
		{
			ui.tableWidget_map->insertRow(i);
		}

	}
	else if (current < max)
	{
		//remove till max
		for (int i = current; i > max; --i)
		{
			ui.tableWidget_map->removeRow(i);
		}
	}
}

void MapForm::onUpdateNpcList(int floor)
{
	npc_hash_.clear();
	ui.tableWidget_map->clear();
	QStringList header = { tr("location"), tr("cod") };
	ui.tableWidget_map->setHorizontalHeaderLabels(header);

	QString key = QString::number(floor);
	QList<util::MapData> datas;

	{
		Injector& injector = Injector::getInstance();
		util::Config config(injector.getPointFileName());
		datas = config.readMapData(key);
		if (datas.isEmpty())
			return;
	}

	int size = datas.size();
	resizeTableWidgetRow(size);
	for (int i = 0; i < size; ++i)
	{
		util::MapData d = datas.at(i);
		QPoint point(d.x, d.y);
		npc_hash_.insert(i, point);

		QString location = d.name;
		QString position = QString("%1,%2").arg(d.x).arg(d.y);
		updateTableWidgetContent(i, 0, location);
		updateTableWidgetContent(i, 1, position);
	}
}

void MapForm::onTableWidgetCellDoubleClicked(int row, int col)
{

	if (!npc_hash_.contains(row))
		return;

	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return;

	if (!injector.server->getOnlineFlag())
		return;

	if (!interpreter_.isNull() && interpreter_->isRunning())
		return;

	interpreter_.reset(new Interpreter());
	connect(interpreter_.data(), &Interpreter::finished, this, &MapForm::onScriptFinished, Qt::UniqueConnection);

	QPoint point = npc_hash_.value(row);
	interpreter_->doString(QString(u8"findpath %1, %2, 1").arg(point.x()).arg(point.y()), nullptr, Interpreter::kNotShare);
	ui.pushButton_findpath_stop->setEnabled(true);
	ui.pushButton_findpath_start->setEnabled(false);
}