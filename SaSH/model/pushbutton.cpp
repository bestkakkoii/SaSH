#include "stdafx.h"
#include "pushButton.h"

PushButton::PushButton(QWidget* parent)
	: QPushButton(parent)
{
	setAttribute(Qt::WA_StyledBackground);

	setStyleSheet(R"(
		QPushButton {
			background-color: #F9F9F9;
			border: 0px solid #000000;
			padding: 3px;
			border-radius: 5px;
		}
		
		QPushButton:hover {
			background-color: #3282F6;
		}
		
		QPushButton:pressed, QPushButton:checked {
			background-color: #3282F6;
			border: 0px solid #ffffff;
		})");

	QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect;
	shadowEffect->setBlurRadius(5); // 設置陰影的模糊半徑，根據需要調整
	shadowEffect->setOffset(1, 1);   // 設置陰影的偏移量，根據需要調整
	shadowEffect->setColor(Qt::black); // 設置陰影的顏色，根據需要調整
	setGraphicsEffect(shadowEffect);

	setMaximumHeight(17);
	setFixedHeight(17);
}