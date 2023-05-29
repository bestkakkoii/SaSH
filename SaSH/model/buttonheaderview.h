#pragma once
#include <QObject>
#include <QWidget>
#include <QHeaderView>
#include <QMouseEvent>

class ButtonHeader : public QHeaderView
{
	Q_OBJECT
public:
	explicit ButtonHeader(Qt::Orientation orientation, QWidget* parent = nullptr)
		: QHeaderView(orientation, parent)
	{
	}

signals:
	void headerClicked(int col);

protected:
	void mousePressEvent(QMouseEvent* e) override
	{
		QHeaderView::mousePressEvent(e);
		int col = logicalIndexAt(e->pos().x());
		emit headerClicked(col);
	}
};