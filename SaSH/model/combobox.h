#pragma once
#ifndef COMBOBOX_H
#define COMBOBOX_H
#pragma execution_character_set("utf-8")
#include <QComboBox>
#include <QMouseEvent>
class ComboBox :
	public QComboBox
{
	Q_OBJECT  //不寫這個沒辦法用信號槽機制，必須得寫
public:
	explicit ComboBox(QWidget* parent = 0);
	virtual ~ComboBox();
protected:
	virtual void mousePressEvent(QMouseEvent* e);  //重寫鼠標點擊事件

signals:
	void clicked();  //自定義點擊信號，在mousePressEvent事件發生時觸發，名字無所謂，隨自己喜歡就行
};
#endif // COMBOBOX_H
