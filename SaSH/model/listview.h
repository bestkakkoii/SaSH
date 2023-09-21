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
#include <QListView>
#include <QStyledItemDelegate>
#include <QReadWriteLock>
#include <QContiguousCache>
#if _MSVC_LANG > 201703L
#include <ranges>
#endif
class StringListModel : public QAbstractListModel
{
	Q_OBJECT
public:
	StringListModel(QObject* parent = nullptr);

	virtual~StringListModel() = default;

	int size() const { QReadLocker locker(&m_stringlistLocker); return m_list.size(); }

	void append(const QString& str, int color = 0);

	QString takeFirst();

	void remove(const QString& str);

	void clear();

	void swapRowUp(int source);

	void swapRowDown(int source);

protected:
	int rowCount(const QModelIndex& parent = QModelIndex()) const override { return parent.isValid() ? 0 : m_list.size(); }

signals:
	void dataAppended();
	void numberPopulated(int number);

private:
	QVector<QString> m_list;
	QVector<qint64> m_colorlist;
	//int listCount = 0;
	mutable QReadWriteLock m_stringlistLocker;

	QVector<QString> getList() const { QReadLocker locker(&m_stringlistLocker); return m_list; }
	void setList(const QVector<QString>& list) { QWriteLocker locker(&m_stringlistLocker); m_list = list; }
	QVector<qint64>getColorList() const { QReadLocker locker(&m_stringlistLocker); return m_colorlist; }
	void setColorList(const QVector<qint64>& list) { QWriteLocker locker(&m_stringlistLocker); m_colorlist = list; }

	void getAllList(QVector<QString>& list, QVector<qint64>& colorlist) const { QReadLocker locker(&m_stringlistLocker); list = m_list; colorlist = m_colorlist; }
	void setAllList(const QVector<QString>& list, const QVector<qint64>& colorlist) { QWriteLocker locker(&m_stringlistLocker); m_list = list; m_colorlist = colorlist; }

	QModelIndex sibling(int row, int column, const QModelIndex& idx) const override { Q_UNUSED(idx); return createIndex(row, column); }

	bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

	bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

	bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild) override;

	QMap<int, QVariant> itemData(const QModelIndex& index) const override;

	bool setItemData(const QModelIndex& index, const QMap<int, QVariant>& roles) override { Q_UNUSED(index); Q_UNUSED(roles); return true; }

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
	void append(const QString& str, int color = 0);
	void remove(const QString& str);
	void clear();
	void swapRowUp(int source);
	void swapRowDown(int source);

protected:
	virtual void wheelEvent(QWheelEvent* event) override;
	virtual bool eventFilter(QObject* obj, QEvent* e) override;
};
