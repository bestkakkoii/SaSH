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
		setAttribute(Qt::WA_StyledBackground);
		setStyleSheet(R"(
QScrollBar:vertical {
	min-height: 30px;  
    background: #F1F1F1; 
}

QScrollBar::handle:vertical {
    background: #2D74DB;
  	border: 3px solid  #F1F1F1;
	min-height:50px;
}

QScrollBar::handle:hover:vertical,
QScrollBar::handle:pressed:vertical {
    background: #3487FF;
}

QScrollBar:horizontal {
    background: #F1F1F1; 
}

QScrollBar::handle:horizontal {
    background: #2D74DB;
  	border: 3px solid  #F1F1F1;
	min-width:50px;
}

QScrollBar::handle:hover:horizontal,
QScrollBar::handle:pressed:horizontal {
    background: #3487FF;
}
		)");


	}
};