﻿#pragma once
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHeaderView>
#include <QVector>

class DragDropWidget : public QTableWidget
{
	Q_OBJECT

public:
	DragDropWidget(QWidget* parent = nullptr)
		: QTableWidget(parent)
	{
		setColumnCount(1);
		setStyleSheet("QTableWidget {font-size:12px;}");
		horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
		horizontalHeader()->setMaximumHeight(14);
		horizontalHeader()->setMinimumHeight(14);
		verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
		setEditTriggers(QAbstractItemView::NoEditTriggers);
		setDragDropMode(QAbstractItemView::DragDrop);
		setSelectionMode(QAbstractItemView::SingleSelection);
		setDefaultDropAction(Qt::DropAction::IgnoreAction);
		setDragEnabled(true);
		setMouseTracking(true);

		qRegisterMetaType<QVector<long long>>("QVector<long long>");
	}

	QStringList getOrder()
	{
		QStringList order;
		long long count = rowCount();
		for (long long row = 0; row < count; ++row)
		{
			QTableWidgetItem* item = this->item(row, 0);
			if (item != nullptr)
			{
				order.append(QString::number(item->data(Qt::UserRole).toLongLong() + 1));
			}
		}
		qDebug() << order;
		return order;
	}

	void reorderRows(const QString& inputString)
	{
		QStringList numbers = inputString.split("|");

		if (numbers.size() != rowCount())
		{
			qDebug() << "Error: Number of numbers does not match the number of rows in the table.";
			return;
		}

		QList<long long> newOrder;
		for (const QString& numberStr : numbers)
		{
			bool ok;
			long long number = numberStr.toLongLong(&ok) - 1;
			if (!ok || number < 0 || number >= rowCount())
			{
				qDebug() << "Error: Invalid number found in input string.";
				return;
			}
			newOrder.append(number);
		}

		// Sort the table rows based on the new order
		for (int row = 0; row < rowCount(); ++row)
		{
			QTableWidgetItem* item = this->item(row, 0);
			if (item != nullptr)
			{
				long long userRole = item->data(Qt::UserRole).toLongLong();
				if (newOrder.value(row) != userRole)
				{
					swapRow(row, newOrder.indexOf(userRole));
				}
			}
		}
	}

public slots:
	void onResetControlTextLanguage();

protected:
	void dropEvent(QDropEvent* event) override
	{
		if (drapRow == -1)
			return;

		const QMimeData* mimeData = event->mimeData();
		if (!mimeData->hasFormat("application/x-qabstractitemmodeldatalist"))
			return;

		QByteArray encodedata = mimeData->data("application/x-qabstractitemmodeldatalist");

		QDataStream stream(&encodedata, QIODevice::ReadOnly);

		long long  row, col;
		QMap<long long, QVariant> roleDataMap;
		stream >> row >> col >> roleDataMap;

		QTableWidgetItem* pDropItem = itemAt(event->pos());
		if (pDropItem == nullptr)
			return;

		long long to = pDropItem->row();

		if (to == -1 || to == drapRow)
			return;

		swapRow(drapRow, to);
		emit orderChanged(getOrder());

	}

	void dragEnterEvent(QDragEnterEvent* event) override
	{
		drapRow = event->source() == this ? currentRow() : -1;
		QTableWidget::dragEnterEvent(event);
	}

signals:
	void orderChanged(const QStringList& order);

private:
	void swapRow(long long  from, long long  to)
	{
		if (from == -1 || to == -1)
			return;

		if (from == to)
			return;

		QTableWidgetItem* itemFrom = takeItem(from, 0);
		QTableWidgetItem* itemTo = takeItem(to, 0);
		setItem(from, 0, itemTo);
		setItem(to, 0, itemFrom);
	}

private:
	long long drapRow = -1;
};
