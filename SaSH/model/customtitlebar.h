#pragma once


class CustomTitleBar : public QWidget
{
	Q_OBJECT
public:
	explicit CustomTitleBar(QWidget* parent = nullptr);
	virtual ~CustomTitleBar();

signals:
	void maximizeClicked();
	void closeClicked();

public:
	void onTitleChanged(const QString& title);

protected:
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
	void toggleMaximize();

private:
	QLabel* titleLabel_ = nullptr;
	QPoint dragStartPosition_ = QPoint();
};