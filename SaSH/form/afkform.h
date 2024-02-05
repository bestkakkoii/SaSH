/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2024 All Rights Reserved.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/

#pragma once
#include <QWidget>
#include "ui_afkform.h"
#include <indexer.h>

class AfkForm : public QWidget, public Indexer
{
	Q_OBJECT

public:
	AfkForm(long long index, QWidget* parent);
	virtual ~AfkForm();

signals:
	void resetControlTextLanguage();

private slots:
	void onButtonClicked();

	void onCheckBoxStateChanged(int state);

	void onSpinBoxValueChanged(int value);

	void onComboBoxCurrentIndexChanged(int value);

	void onLineEditTextChanged(const QString& text);

	void onComboBoxClicked();

	void onComboBoxTextChanged(const QString& text);

	void onApplyHashSettingsToUI();

	void onResetControlTextLanguage();

	void onUpdateComboBoxItemText(long long type, const QStringList& textList);

	void onDragDropWidgetItemChanged(const QStringList& order);

protected:
	virtual void showEvent(QShowEvent* e) override;

	virtual void closeEvent(QCloseEvent* event) override;

	virtual bool eventFilter(QObject* obj, QEvent* eve) override;

	virtual void hideEvent(QHideEvent* event) override;

private:
	void updateTargetButtonText();


private:
	Ui::AfkFormClass ui;
	QWidget* pparent = nullptr;
};
