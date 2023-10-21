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

MapForm::MapForm(long long index, QWidget* parent)
	: QWidget(parent)
	, Indexer(index)
{
	ui.setupUi(this);
	setFont(util::getFont());

	qRegisterMetaType<QVariant>("QVariant");
	qRegisterMetaType<QVariant>("QVariant&");

	ui.tableWidget_map->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui.tableWidget_map->horizontalHeader()->setStretchLastSection(true);

	QList<PushButton*> buttonList = util::findWidgets<PushButton>(this);
	for (auto& button : buttonList)
	{
		if (button)
		{
			util::setPushButton(button);
			connect(button, &PushButton::clicked, this, &MapForm::onButtonClicked, Qt::QueuedConnection);
		}
	}

	connect(ui.tableWidget_map, &QTableWidget::cellDoubleClicked, this, &MapForm::onTableWidgetCellDoubleClicked, Qt::QueuedConnection);

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(index);
	connect(&signalDispatcher, &SignalDispatcher::updateNpcList, this, &MapForm::onUpdateNpcList, Qt::QueuedConnection);

	QList <QSpinBox*> spinBoxList = util::findWidgets<QSpinBox>(this);
	for (auto& spinBox : spinBoxList)
	{
		if (spinBox)
			util::setSpinBox(spinBox);
	}

	util::setTableWidget(ui.tableWidget_map);

}

MapForm::~MapForm()
{
	long long currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	injector.IS_FINDINGPATH.store(false, std::memory_order_release);
}

void MapForm::showEvent(QShowEvent* e)
{
	setAttribute(Qt::WA_Mapped);
	QWidget::showEvent(e);
}

void MapForm::onFindPathFinished()
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

	long long currentIndex = getIndex();

	if (name == "pushButton_findpath_start")
	{
		Injector& injector = Injector::getInstance(currentIndex);
		if (!injector.isValid())
			return;

		if (injector.IS_SCRIPT_FLAG.load(std::memory_order_acquire))
			return;

		if (injector.IS_FINDINGPATH.load(std::memory_order_acquire))
			return;

		if (injector.worker.isNull())
			return;

		if (!injector.worker->getOnlineFlag())
			return;

		if (findPathFuture_.isRunning())
		{
			injector.IS_FINDINGPATH.store(false, std::memory_order_release);
			findPathFuture_.cancel();
			findPathFuture_.waitForFinished();
		}

		connect(injector.worker.get(), &Worker::findPathFinished, this, &MapForm::onFindPathFinished, Qt::UniqueConnection);

		long long x = ui.spinBox_findpath_x->value();
		long long y = ui.spinBox_findpath_y->value();

		QPoint point(x, y);
		findPathFuture_ = QtConcurrent::run(injector.worker.get(), &Worker::findPathAsync, point);

		ui.pushButton_findpath_stop->setEnabled(true);
		ui.pushButton_findpath_start->setEnabled(false);
	}
	else if (name == "pushButton_findpath_stop")
	{
		long long currentIndex = getIndex();
		Injector& injector = Injector::getInstance(currentIndex);
		injector.IS_FINDINGPATH.store(false, std::memory_order_release);
		ui.pushButton_findpath_stop->setEnabled(false);
		ui.pushButton_findpath_start->setEnabled(true);
	}
}

void MapForm::resizeTableWidgetRow(long long max)
{
	//set row count
	ui.tableWidget_map->setRowCount(max);
	long long current = ui.tableWidget_map->rowCount();
	if (current > max)
	{
		//insert till max
		for (long long i = current; i < max; ++i)
		{
			ui.tableWidget_map->insertRow(i);
		}

	}
	else if (current < max)
	{
		//remove till max
		for (long long i = current; i > max; --i)
		{
			ui.tableWidget_map->removeRow(i);
		}
	}
}

void MapForm::onUpdateNpcList(long long floor)
{
	npc_hash_.clear();
	ui.tableWidget_map->clear();
	QStringList header = { tr("location"), tr("cod") };
	ui.tableWidget_map->setHorizontalHeaderLabels(header);

	QString key = util::toQString(floor);
	QList<util::MapData> datas;
	long long currentIndex = getIndex();
	{
		Injector& injector = Injector::getInstance(currentIndex);
		util::Config config(injector.getPointFileName(), QString("%1|%2").arg(__FUNCTION__).arg(__LINE__));
		datas = config.readMapData(key);
		if (datas.isEmpty())
			return;
	}

	long long size = datas.size();
	resizeTableWidgetRow(size);
	for (long long i = 0; i < size; ++i)
	{
		util::MapData d = datas.value(i);
		QPoint point(d.x, d.y);
		npc_hash_.insert(i, point);

		QString location = d.name;
		QString position = QString("%1,%2").arg(d.x).arg(d.y);
		ui.tableWidget_map->setText(i, 0, location);
		ui.tableWidget_map->setText(i, 1, position);
	}
}

void MapForm::onTableWidgetCellDoubleClicked(int row, int col)
{
	if (!npc_hash_.contains(row))
		return;

	long long currentIndex = getIndex();
	Injector& injector = Injector::getInstance(currentIndex);
	if (!injector.isValid())
		return;

	if (injector.IS_SCRIPT_FLAG.load(std::memory_order_acquire))
		return;

	if (injector.IS_FINDINGPATH.load(std::memory_order_acquire))
		return;

	if (injector.worker.isNull())
		return;

	if (!injector.worker->getOnlineFlag())
		return;

	if (findPathFuture_.isRunning())
	{
		injector.IS_FINDINGPATH.store(false, std::memory_order_release);
		findPathFuture_.cancel();
		findPathFuture_.waitForFinished();
	}

	connect(injector.worker.get(), &Worker::findPathFinished, this, &MapForm::onFindPathFinished, Qt::UniqueConnection);

	QPoint point = npc_hash_.value(row);
	findPathFuture_ = QtConcurrent::run(injector.worker.get(), &Worker::findPathAsync, point);

	ui.pushButton_findpath_stop->setEnabled(true);
	ui.pushButton_findpath_start->setEnabled(false);
}