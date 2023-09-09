#pragma once

#include <QPushButton>

class PushButton : public QPushButton
{
	Q_OBJECT
public:
	explicit PushButton(QWidget* parent = nullptr);

	virtual ~PushButton() = default;
};