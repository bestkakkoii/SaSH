#include "stdafx.h"
#include "pushButton.h"

PushButton::PushButton(QWidget* parent)
	: QPushButton(parent)
{
	setAttribute(Qt::WA_StyledBackground);

	//setStyleSheet(R"(
	//	QPushButton {
	//		background-color: #F9F9F9;
	//		border: 0px solid #000000;
	//		padding: 3px;
	//		border-radius: 5px;
	//	}
	//	
	//	QPushButton:hover {
	//		background-color: #3282F6;
	//	}
	//	
	//	QPushButton:pressed, QPushButton:checked {
	//		background-color: #3282F6;
	//		border: 0px solid #ffffff;
	//	})");

	//setMaximumHeight(20);
	//setFixedHeight(20);
}