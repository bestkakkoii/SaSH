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

	~StringListModel() = default;

	void append(const QString& str, int color = 0);
	//void append(const QStringList& str);

	int size() const { QReadLocker locker(&m_stringlistLocker); return m_list.size(); }

	QString takeFirst();

	void remove(const QString& str);

	void clear();

	//void setStringList(const QStringList& list);

	//QStringList getStringList() const { QReadLocker locker(&m_stringlistLocker); return m_list.toList(); }


	void swapRowUp(int source);
	void swapRowDown(int source);
protected:
	//bool canFetchMore(const QModelIndex& parent) const override;
	//void fetchMore(const QModelIndex& parent) override;
	int rowCount(const QModelIndex& parent = QModelIndex()) const override { return parent.isValid() ? 0 : m_list.size(); }

signals:
	void dataAppended();
	void numberPopulated(int number);

private:
	QVector<QString> m_list;
	QVector<int> m_colorlist;
	//int listCount = 0;
	mutable QReadWriteLock m_stringlistLocker;

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
	ListView(QWidget* parent = nullptr);

	~ListView() = default;

	//如果數據改變則滾動到底部
	void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>()) override;

	void setModel(StringListModel* model);


};
