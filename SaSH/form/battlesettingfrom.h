#pragma once

#include <QWidget>
#include "ui_battlesettingfrom.h"
#include "indexer.h"

class BattleSettingFrom : public QWidget, public Indexer
{
	Q_OBJECT

public:
	BattleSettingFrom(long long index, QWidget* parent);
	virtual ~BattleSettingFrom();

private slots:
	void onListWidgetAddItem(const QString& text);
	void onTextBrowserAppendText(const QString& text, bool cacheOnly);
	void onButtonClicked();
	void onListWidgetDoubleClicked(QListWidgetItem* item);
private:
	Ui::BattleSettingFromClass ui;

	QString cacheText = "";

};

class BattleConditionTextItem : public QGraphicsTextItem
{
	Q_OBJECT

signals:
	void clicked(const QString& text, bool cacheOnly);
public:
	explicit BattleConditionTextItem(const QString& text, QGraphicsItem* parent = nullptr);

protected:
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* e) override;

	virtual void mousePressEvent(QGraphicsSceneMouseEvent* e) override;

	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* e) override;

	virtual void dragEnterEvent(QGraphicsSceneDragDropEvent* e) override;

	virtual void dropEvent(QGraphicsSceneDragDropEvent* e) override;

	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

	virtual void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;

	virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
	QString text_;
	bool isHovered_ = false;
	bool isPressed_ = false;
	qreal width_ = 80.0; // 调整按钮宽度
	qreal height_ = 20.0; // 调整按钮高度
	QFont font_;

};