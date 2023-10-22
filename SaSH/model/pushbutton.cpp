#include "stdafx.h"
#include "pushButton.h"

PushButton::PushButton(QWidget* parent)
	: QPushButton(parent)
{
	setAttribute(Qt::WA_StyledBackground);

	setStyleSheet(R"(
		QPushButton {
			background-color: white;
			border: 1px solid gray;
			border-radius: 5px;
			padding: 2px;
			color: black;
		}
		
		QPushButton:hover {
			background-color: white;
			border: 1px solid #4096FF;
			color:#4096FF;
		}
		
		QPushButton:pressed, QPushButton:checked {
			background-color: white;
			border: 1px solid #0958D9;
			color:#0958D9;
		}

		QPushButton:disabled {
			background-color: #F0F4F8;
			color:gray;
		}

		)");

	setFixedHeight(19);
}