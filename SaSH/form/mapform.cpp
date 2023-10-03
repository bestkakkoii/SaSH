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


MapForm::MapForm(qint64 index, QWidget* parent)
	: QWidget(parent)
	, Indexer(index)
{
	ui.setupUi(this);

	qRegisterMetaType<QVariant>("QVariant");
	qRegisterMetaType<QVariant>("QVariant&");

	QList<PushButton*> buttonList = util::findWidgets<PushButton>(this);
	for (auto& button : buttonList)
	{
		if (button)
			connect(button, &PushButton::clicked, this, &MapForm::onButtonClicked, Qt::UniqueConnection);
	}

	connect(ui.tableWidget_map, &QTableWidget::cellDoubleClicked, this, &MapForm::onTableWidgetCellDoubleClicked, Qt::UniqueConnection);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
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
	PushButton* pPushButton = qobject_cast<PushButton*>(sender());
	if (!pPushButton)
		return;

	QString name = pPushButton->objectName();
	if (name.isEmpty())
		return;

	qint64 currentIndex = getIndex();

	if (name == "pushButton_findpath_start")
	{
		Injector& injector = Injector::getInstance(currentIndex);
		if (injector.server.isNull())
			return;

		if (!injector.server->getOnlineFlag())
			return;

		if (!interpreter_.isNull() && interpreter_->isRunning())
		{
			interpreter_->stop();
			return;
		}

		interpreter_.reset(new Interpreter(currentIndex));
		connect(interpreter_.data(), &Interpreter::finished, this, &MapForm::onScriptFinished);

		qint64 x = ui.spinBox_findpath_x->value();
		qint64 y = ui.spinBox_findpath_y->value();

		interpreter_->doString(QString("findpath(%1, %2, 3)").arg(x).arg(y), nullptr, Interpreter::kNotShare);
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

void MapForm::updateTableWidgetContent(qint64 row, qint64 col, const QString& text)
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

void MapForm::resizeTableWidgetRow(qint64 max)
{
	//set row count
	ui.tableWidget_map->setRowCount(max);
	qint64 current = ui.tableWidget_map->rowCount();
	if (current > max)
	{
		//insert till max
		for (qint64 i = current; i < max; ++i)
		{
			ui.tableWidget_map->insertRow(i);
		}

	}
	else if (current < max)
	{
		//remove till max
		for (qint64 i = current; i > max; --i)
		{
			ui.tableWidget_map->removeRow(i);
		}
	}
}

void MapForm::onUpdateNpcList(qint64 floor)
{
	npc_hash_.clear();
	ui.tableWidget_map->clear();
	QStringList header = { tr("location"), tr("cod") };
	ui.tableWidget_map->setHorizontalHeaderLabels(header);

	QString key = util::toQString(floor);
	QList<util::MapData> datas;
	qint64 currentIndex = getIndex();
	{
		Injector& injector = Injector::getInstance(currentIndex);
		util::Config config(injector.getPointFileName());
		datas = config.readMapData(key);
		if (datas.isEmpty())
			return;
	}

	qint64 size = datas.size();
	resizeTableWidgetRow(size);
	for (qint64 i = 0; i < size; ++i)
	{
		util::MapData d = datas.value(i);
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

	qint64 currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (injector.server.isNull())
		return;

	if (!injector.server->getOnlineFlag())
		return;

	if (!interpreter_.isNull() && interpreter_->isRunning())
	{
		interpreter_->stop();
		return;
	}

	interpreter_.reset(new Interpreter(currentIndex));
	connect(interpreter_.data(), &Interpreter::finished, this, &MapForm::onScriptFinished);

	QPoint point = npc_hash_.value(row);
	interpreter_->doString(QString("findpath(%1, %2, 3)").arg(point.x()).arg(point.y()), nullptr, Interpreter::kNotShare);
	ui.pushButton_findpath_stop->setEnabled(true);
	ui.pushButton_findpath_start->setEnabled(false);
}