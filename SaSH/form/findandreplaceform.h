#pragma once

#include <QWidget>
#include "ui_findandreplaceform.h"

class FindAndReplaceForm : public QWidget
{
	Q_OBJECT

public:
	FindAndReplaceForm(QWidget* parent = nullptr);
	~FindAndReplaceForm();

	void setSearchText(const QString& text);

signals:
	void findFirsted(const QString& expr, bool re, bool cs, bool wo, bool wrap, bool forward = true, int line = -1, int index = -1, bool show = true, bool posix = false);
	void findNexted();
	void replaceAlled(const QString& replacetext, const QString& expr, bool re, bool cs, bool wo, bool wrap, bool forward = true, int line = -1, int index = -1, bool show = true, bool posix = false);
	void findAlled(const QString& expr, bool re, bool cs, bool wo, bool wrap, bool forward = true, int line = -1, int index = -1, bool show = true, bool posix = false);

private slots:
	void onButtonClicked();
	void onComboBoxTextChanged(const QString& text);

protected:
	void showEvent(QShowEvent* event) override;
	void closeEvent(QCloseEvent* event) override;

private:
	Ui::FindAndReplaceFormClass ui;

	bool isFirstFindNext_ = true;
	bool isFirstFindPrev_ = true;
};
