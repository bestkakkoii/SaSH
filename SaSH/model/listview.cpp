#include "stdafx.h"
#include <QApplication>
#include <listview.h>
#include <qmath.h>

constexpr int MAX_LIST_COUNT = 1024;

ListView::ListView(QWidget* parent)
	: QListView(parent)
{
}

void ListView::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
	QListView::dataChanged(topLeft, bottomRight, roles);
	//如果數據改變則滾動到底部
	//scrollToBottom();
}

void ListView::setModel(StringListModel* model)
{
	if (model)
	{
		StringListModel* old_mod = (StringListModel*)this->model();
		if (old_mod)
			disconnect(old_mod, &StringListModel::dataAppended, this, &QListView::scrollToBottom);
		QListView::setModel(model);
		connect(model, &StringListModel::dataAppended, this, &QListView::scrollToBottom);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
StringListModel::StringListModel(QObject* parent)
	: QAbstractListModel(parent)
{
}


void StringListModel::append(const QString& str, int color)
{
	QWriteLocker locker(&m_stringlistLocker);
	beginInsertRows(QModelIndex(), rowCount(), rowCount());
	if (m_list.size() >= MAX_LIST_COUNT)
	{
		m_list.clear();
		m_colorlist.clear();
	}
	m_list.append(str);
	m_colorlist.append(color);
	endInsertRows();
	emit dataChanged(index(rowCount() - 1), index(rowCount() - 1), QVector<int>() << Qt::DisplayRole);
	emit dataAppended();
}

//void StringListModel::append(const QStringList& strs)
//{
//	QWriteLocker locker(&m_stringlistLocker);
//	beginInsertRows(QModelIndex(), rowCount(), rowCount());
//	if (m_list.size() >= MAX_LIST_COUNT)
//	{
//		m_list.clear();
//		//listCount = 0;
//	}
//	m_list.append(strs.toVector());
//	endInsertRows();
//	emit dataChanged(index(rowCount() - 1), index(rowCount() - 1), QVector<int>() << Qt::DisplayRole);
//	emit dataAppended();
//}

QString StringListModel::takeFirst()
{
	QWriteLocker locker(&m_stringlistLocker);
	if (m_list.size() > 0)
	{
		beginRemoveRows(QModelIndex(), 0, 0);
		QString str = m_list.takeFirst();
		endRemoveRows();
		return str;
	}
	return QString();
}

void StringListModel::remove(const QString& str)
{
	QWriteLocker locker(&m_stringlistLocker);
	int index = m_list.indexOf(str);
	if (index != -1)
	{
		beginRemoveRows(QModelIndex(), index, index);
		m_list.removeAt(index);
		m_colorlist.removeAt(index);
		endRemoveRows();
	}
}

//void StringListModel::setStringList(const QStringList& list)
//{
//	QWriteLocker locker(&m_stringlistLocker);
//	beginResetModel();
//	m_list = list.toVector();
//	endResetModel();
//	emit dataAppended();
//}

void StringListModel::clear()
{
	QWriteLocker locker(&m_stringlistLocker);
	beginResetModel();
	m_list.clear();
	m_colorlist.clear();
	//listCount = 0;
	endResetModel();
}

// 用于排序的比较函数，按整数值进行比较
bool pairCompare(const QPair<int, QString>& pair1, const QPair<int, QString>& pair2)
{
	return pair1.first < pair2.first;
}

bool pairCompareGreaerString(const QPair<int, QString>& pair1, const QPair<int, QString>& pair2)
{
	return pair1.second > pair2.second;
}
void StringListModel::sort(int column, Qt::SortOrder order)
{
	Q_UNUSED(column);
	QWriteLocker locker(&m_stringlistLocker);
	beginResetModel();
	if (order == Qt::AscendingOrder)
	{

		QVector<QPair<int, QString>> pairVector;
		for (int i = 0; i < m_colorlist.size(); ++i)
		{
			pairVector.append(qMakePair(m_colorlist[i], m_list[i]));
		}

		std::sort(pairVector.begin(), pairVector.end(), pairCompare);

		m_list.clear();
		m_colorlist.clear();

		for (int i = 0; i < pairVector.size(); ++i)
		{
			m_colorlist.append(pairVector[i].first);
			m_list.append(pairVector[i].second);
		}
	}
	else
	{
		QVector<QPair<int, QString>> pairVector;
		for (int i = 0; i < m_colorlist.size(); ++i)
		{
			pairVector.append(qMakePair(m_colorlist[i], m_list[i]));
		}

		std::sort(pairVector.begin(), pairVector.end(), pairCompareGreaerString);

		m_list.clear();
		m_colorlist.clear();

		for (int i = pairVector.size() - 1; i >= 0; --i)
		{
			m_colorlist.append(pairVector[i].first);
			m_list.append(pairVector[i].second);
		}

	}
	endResetModel();
}

bool StringListModel::insertRows(int row, int count, const QModelIndex& parent)
{
	Q_UNUSED(parent);
	beginInsertRows(QModelIndex(), row, row + count - 1);
	endInsertRows();
	return true;
}
bool StringListModel::removeRows(int row, int count, const QModelIndex& parent)
{
	Q_UNUSED(parent);
	beginRemoveRows(QModelIndex(), row, row + count - 1);
	endRemoveRows();
	return true;
}
bool StringListModel::moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild)
{
	Q_UNUSED(destinationParent);
	Q_UNUSED(sourceParent);
	beginMoveRows(QModelIndex(), sourceRow, sourceRow + count - 1, QModelIndex(), destinationChild);
	endMoveRows();
	return true;
}

QMap<int, QVariant> StringListModel::itemData(const QModelIndex& index) const
{
	QMap<int, QVariant> map;
	map.insert(Qt::DisplayRole, m_list.at(index.row()));
	return map;
}

QVariant StringListModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();


	switch (role)
	{
	case Qt::DisplayRole:
	{
		if (index.row() >= m_list.size() || index.row() < 0)
			return QVariant();
		return m_list.at(index.row());
	}
	case Qt::BackgroundRole:
	{
		static const QBrush brushBase = qApp->palette().base();
		static const QBrush brushAlternate = qApp->palette().alternateBase();
		return ((index.row() & 99) == 0) ? brushBase : brushAlternate;
		/*int batch = (index.row() / 100) % 2;
		if (batch == 0)
			return brushBase;
		else
			return brushAlternate;*/
	}
	case Qt::ForegroundRole:
	{

		static const QHash<int, QColor> hash = {
			{ 0, QColor(255,255,255) },
			{ 1, QColor(0,255,255) },
			{ 2, QColor(255,0,255) },
			{ 3, QColor(0,0,255) },
			{ 4, QColor(255,255,0) },
			{ 5, QColor(0,255,0) },
			{ 6, QColor(255,0,0) },
			{ 7, QColor(160,160,164) },
			{ 8, QColor(166,202,240) },
			{ 9, QColor(192,220,192) },
			{ 10, QColor(218,175,66) },
		};
		if (index.row() >= m_list.size() || index.row() < 0)
			return QColor(255, 255, 255);

		int color = m_colorlist.at(index.row());
		QColor c = hash.value(color, QColor(255, 255, 255));
		return c;
	}
	default: break;
	}
	return QVariant();
}

bool StringListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (role == Qt::EditRole)
	{
		if (index.row() >= 0 && index.row() < m_list.size())
		{
			m_list.replace(index.row(), value.toString());
			emit dataChanged(index, index, QVector<int>() << role);
			return true;
		}
	}
	return false;
}

void StringListModel::swapRowUp(int source)
{
	if (source > 0)
	{
		QWriteLocker locker(&m_stringlistLocker);
		beginMoveRows(QModelIndex(), source, source, QModelIndex(), source - 1);
		m_list.swapItemsAt(source, source - 1);
		m_colorlist.swapItemsAt(source, source - 1);
		endMoveRows();
	}
}
void StringListModel::swapRowDown(int source)
{
	if (source + 1 < m_list.size())
	{
		QWriteLocker locker(&m_stringlistLocker);
		beginMoveRows(QModelIndex(), source, source, QModelIndex(), source + 2);
		m_list.swapItemsAt(source, source + 1);
		m_colorlist.swapItemsAt(source, source + 1);
		endMoveRows();
	}
}