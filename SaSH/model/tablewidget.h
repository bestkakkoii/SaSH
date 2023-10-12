#pragma once
#include <QObject>
#include <QTableWidget>
#include <QHeaderView>

class TableWidget : public QTableWidget
{
	Q_OBJECT
public:
	explicit TableWidget(QWidget* parent = nullptr);

	virtual ~TableWidget() = default;

	void setItemForeground(__int64 row, __int64 column, const QColor& color);

	void setText(__int64 row, __int64 column, const QString& text, const QString& toolTip = "", QVariant data = QVariant());

	void setHorizontalHeaderText(__int64 col, const QString& text);

private:
	void setItem(__int64 row, __int64 column, QTableWidgetItem* item);
};