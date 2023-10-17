#pragma once

#include <QObject>
#include <QListWidget>

class ListWidget : public QListWidget
{
	Q_OBJECT
public:

	explicit ListWidget(QWidget* parent = nullptr)
		: QListWidget(parent)
	{
		setFont(util::getFont());
		setAttribute(Qt::WA_StyledBackground);
		setStyleSheet(R"(
QListWidget{
	color:black;
	background-color: white;
}

QScrollBar:vertical {
	min-height: 30px;  
    background: white; 
}

QScrollBar::handle:vertical {
    background: white;
  	border: 3px solid  white;
	min-height:30px;
}

QScrollBar::handle:hover:vertical,
QScrollBar::handle:pressed:vertical {
    background: #3487FF;
}

QScrollBar:horizontal {
    background: white; 
}

QScrollBar::handle:horizontal {
    background: #3282F6;
  	border: 3px solid white;
	min-width:50px;
}

QScrollBar::handle:hover:horizontal,
QScrollBar::handle:pressed:horizontal {
    background: #3487FF;
}
		)");


	}
};