#include "stdafx.h"
#include "pushButton.h"

PushButton::PushButton(QWidget* parent)
	: QPushButton(parent)
{
	setAttribute(Qt::WA_StyledBackground);

	setStyleSheet(R"(
		QPushButton {
			background-color: #F0F4F8;
			border: 1px solid #000000;
			border-radius: 1px;
			padding: 2px;
			color: #000000;
		}
		
		QPushButton:hover {
			background-color: #006CD6;
			color:#DFEBF6;
		}
		
		QPushButton:pressed, QPushButton:checked {
			background-color: #0080FF;
			color:#DFEBF6;
		}

		QPushButton:disabled {
			background-color: #F0F4F8;
			color:gray;
		}

		)");

	setFixedHeight(19);
}