#pragma once

class CustomTitleBar : public QWidget
{
	Q_OBJECT
public:
	explicit CustomTitleBar(QWidget* parent = nullptr);
	virtual ~CustomTitleBar();

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
};