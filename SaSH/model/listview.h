/*
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
#include <QListView>
#include <QStyledItemDelegate>
#include <QReadWriteLock>
#include <QContiguousCache>
#if _MSVC_LANG > 201703L
#include <ranges>
#endif
#include "safe.h"

class StringListModel : public QAbstractListModel
{
	Q_OBJECT
public:
	StringListModel(QObject* parent = nullptr);

	virtual~StringListModel() = default;

	long long size() const { return list_.size(); }

	void append(const QString& str, long long color = 0);

	QString takeFirst();

	void remove(const QString& str);

	void clear();

	void swapRowUp(long long source);

	void swapRowDown(long long source);

	void setMaxListCount(long long count) { MAX_LIST_COUNT = count; }

protected:
	int rowCount(const QModelIndex& parent = QModelIndex()) const override { return parent.isValid() ? 0 : list_.size(); }

signals:
	void dataAppended();
	void numberPopulated(int number);

private:
	safe::vector<QString> list_;
	safe::vector<long long> colorlist_;
	long long MAX_LIST_COUNT = 512;

	QVector<QString> getList() const { return list_.toVector(); }
	void setList(const QVector<QString>& list) { list_ = list; }
	QVector<long long>getColorList() const { return colorlist_.toVector(); }
	void setColorList(const QVector<long long>& list) { colorlist_ = list; }

	void getAllList(QVector<QString>& list, QVector<long long>& colorlist) const { list = list_.toVector(); colorlist = colorlist_.toVector(); }
	void setAllList(const QVector<QString>& list, const QVector<long long>& colorlist) { list_ = list; colorlist_ = colorlist; }

	QModelIndex sibling(int row, int column, const QModelIndex& idx) const override { std::ignore = idx; return createIndex(row, column); }

	bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

	bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

	bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild) override;

	QMap<int, QVariant> itemData(const QModelIndex& index) const override;

	bool setItemData(const QModelIndex& index, const QMap<int, QVariant>& roles) override { std::ignore = index; std::ignore = roles; return true; }

	void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

	Qt::DropActions supportedDropActions() const override { return Qt::MoveAction; }


	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

	Qt::ItemFlags flags(const QModelIndex& index) const override { return Qt::ItemIsEditable | QAbstractListModel::flags(index); }
};

class ListView : public QListView
{
	Q_OBJECT
public:
	explicit ListView(QWidget* parent = nullptr);

	virtual ~ListView() = default;

	//如果數據改變則滾動到底部
	void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>()) override;

	void setModel(StringListModel* model);

private slots:
	void append(const QString& str, long long color = 0);
	void remove(const QString& str);
	void clear();
	void swapRowUp(long long source);
	void swapRowDown(long long source);

protected:
	virtual void wheelEvent(QWheelEvent* event) override;
	virtual bool eventFilter(QObject* obj, QEvent* e) override;
};
