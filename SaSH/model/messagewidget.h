#ifndef MESSAGEWIDGET_H
#define MESSAGEWIDGET_H

#include <QWidget>

class QLabel;
class QPushButton;

class QMessageWidget : public QWidget
{
	Q_OBJECT

public:
	explicit QMessageWidget(const QString& text, QWidget* parent = nullptr);
	virtual ~QMessageWidget();

	void setText(const QString& text);

	QPushButton* getCustonButton() const { return pMessageLabel_; }
protected:
	virtual void paintEvent(QPaintEvent* event) override;

private:
	QPushButton* pMessageLabel_ = nullptr;
};

#endif // MESSAGEWIDGET_H
