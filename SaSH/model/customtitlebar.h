#pragma once

class CustomTitleBar : public QWidget
{
	Q_OBJECT
public:
	enum Button
	{
		kMinimizeButton = 0x01,
		kMaximizeButton = 0x02,
		kCloseButton = 0x04,
		kAllButton = 0x07,
	};

	explicit CustomTitleBar(DWORD button = kAllButton, QWidget* parent = nullptr);
	virtual ~CustomTitleBar();

	QVBoxLayout* getMenuLayout() const { return menuLayout_; }

	void setMenu(QMenu* menu) { menu_ = menu; }

public:
	void onTitleChanged(const QString& title);

protected:
	virtual void showEvent(QShowEvent* e) override
	{
		setAttribute(Qt::WA_Mapped);
		QWidget::showEvent(e);
	}

	virtual void mouseDoubleClickEvent(QMouseEvent* event) override;
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	virtual void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
	void toggleMaximize();


private:
	QLabel* titleLabel_ = nullptr;
	QPushButton* maximizeButton_ = nullptr;
	QPoint dragStartPosition_ = QPoint();
	QWidget* parent_ = nullptr;
	QVBoxLayout* menuLayout_ = nullptr;
	QMenu* menu_ = nullptr;
};