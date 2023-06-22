#pragma once
#include <QDialog>
class CodeEditor;
class FindDialog : public QDialog
{
	Q_OBJECT

protected:
	QGroupBox m_radioGrpBx;

	QGridLayout m_layout;
	QHBoxLayout m_hbLayout;

	QLabel m_findLbl;
	QLineEdit m_findEdit;
	QPushButton m_findBtn;
	QPushButton m_closeBtn;
	QCheckBox m_matchChkBx;
	QRadioButton m_forwardBtn;
	QRadioButton m_backwardBtn;

	CodeEditor* m_pText; // FindDialog 聚合使用 QPlainTextEdit

	void initControl();
	void connectSlot();
protected slots:
	void onFindClicked();
	void onCloseClicked();
public:
	explicit FindDialog(CodeEditor* textWidget);
	void setPlainTextEdit(CodeEditor* pText);
	CodeEditor* getPlainTextEdit();
	bool event(QEvent* evt);


};

class ReplaceDialog : public FindDialog
{
	Q_OBJECT

protected:
	QLabel m_replaceLbl;
	QLineEdit m_replaceEdit;
	QPushButton m_replaceBtn;
	QPushButton m_replaceAllBtn;

	void initControl();
	void connectSlot();
protected slots:
	void onReplaceClicked();
	void onReplaceAllClicked();
public:
	explicit ReplaceDialog(CodeEditor* pText = 0);
};

