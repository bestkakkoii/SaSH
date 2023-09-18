#include "stdafx.h"
#include <codeeditor.h>
#include <QApplication>
#include <QTextCursor>
#include <QTextStream>
#include <QTextEdit>
#include <QFile>
#include <QKeyEvent>
#include <qdialogbuttonbox.h>

#ifdef _DEBUG
#pragma comment(lib, "qscintilla2_qt5d.lib")
#else
#pragma comment(lib, "qscintilla2_qt5.lib")
#endif

CodeEditor::CodeEditor(QWidget* parent)
	: QsciScintilla(parent)
	, textLexer(this)
	, apis(&textLexer)

{
	//install font
	QFontDatabase::addApplicationFont("YaHei Consolas Hybrid 1.12.ttf");
	QFont _font("YaHei Consolas Hybrid", 12, 570/*QFont::DemiBold*/, false);
	setFont(_font);
	font = _font;

	textLexer.setDefaultFont(font);
	setLexer(&textLexer);
	SendScintilla(QsciScintilla::SCI_SETCODEPAGE, QsciScintilla::SC_CP_UTF8);//設置編碼為UTF-8
	setUtf8(true);//設置編碼為UTF-8
	setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

	QsciScintilla::setFont(font);
	//代碼提示autoCompletion
#ifdef _DEBUG
	QFile f(R"(..\Debug\lib\completion_api.txt)");
#else
	QFile f(R"(.\lib\completion_api.txt)");
#endif

	if (f.open(QIODevice::ReadOnly))
	{
		QTextStream ts(&f);
		ts.setCodec("UTF-8");

		for (;;)
		{
			QString line(ts.readLine());

			if (line.isEmpty())
				break;
			apis.add(line.simplified());
		}
		f.close();
		apis.prepare();
	}

	////Acs[None|All|Document|APIs]禁用自動補全提示功能|所有可用的資源|當前文檔中出現的名稱都自動補全提示|使用QsciAPIs類加入的名稱都自動補全提示
	setAutoCompletionCaseSensitivity(true);//大小寫敏感度，設置lexer可能會更改，不過貌似沒啥效果
	setAutoCompletionReplaceWord(false);//是否用補全的字符串替代光標右邊的字符串
	setAutoCompletionShowSingle(false);//則當自動完成清單中只有一個條目時，將自動使用該條目
	setAutoCompletionSource(QsciScintilla::AcsAll);//自動補全。對於所有Ascii字符
	setAutoCompletionThreshold(1);//設置每輸入一個字符就會出現自動補全的提示

	/*
		AcusNever
		The single entry is not used automatically and the auto-completion list is displayed.

		AcusExplicit
		The single entry is used automatically when auto-completion is explicitly requested (using autoCompleteFromAPIs() or autoCompleteFromDocument()) but not when auto-completion is triggered as the user types.

		AcusAlways
		The single entry is used automatically and the auto-completion list is not displayed.
	*/
	setAutoCompletionUseSingle(QsciScintilla::AcusNever);//當自動完成清單中只有一個條目時，將自動使用該條目

	setAutoIndent(true);//換行後自動縮進
	setBraceMatching(QsciScintilla::StrictBraceMatch);//括號匹配
	setBackspaceUnindents(true);//退格鍵將取消縮進一行而不是刪除一個字符

	//current line color
	setCaretLineBackgroundColor(QColor(71, 71, 71));//光標所在行背景顏色
	setCaretLineVisible(true); //是否高亮顯示光標所在行
	setCaretWidth(2);//光標寬度，0表示不顯示光標
	setCaretForegroundColor(QColor(174, 175, 173));  //光標顏色

	setEolMode(QsciScintilla::EolWindows); //微軟風格換行符
	setEolVisibility(false);//是否顯示換行符號

	setAutoCompletionFillupsEnabled(true);

	setCallTipsVisible(0);//顯示tip數量
	setCallTipsHighlightColor(QColor(61, 61, 61));
	setCallTipsStyle(QsciScintilla::CallTipsContext);
	setCallTipsPosition(QsciScintilla::CallTipsBelowText);
	setCallTipsBackgroundColor(QColor(30, 30, 30));

	setAnnotationDisplay(QsciScintilla::AnnotationBoxed);//註釋樣式

	//行號提示
	setFont(font);//設置文本字體
	setWrapMode(QsciScintilla::WrapWord); //文本自動換行模式
	setWrapVisualFlags(QsciScintilla::WrapFlagByText);

	setIndentationsUseTabs(false);//false用表示用空格代替\t
	setIndentationGuides(true);//用tab鍵縮進時，在縮進位置上顯示一個豎點線，縮進有效，在字符串後加空格不顯示
	setIndentationWidth(0);//如果在行首部空格位置tab，縮進的寬度字符數，並且不會轉換為空格

	// 折疊標簽樣式
	setFolding(QsciScintilla::BoxedTreeFoldStyle);//折疊樣式
	setFoldMarginColors(QColor(165, 165, 165), QColor(61, 61, 61));//折疊欄顏色

	setTabIndents(true);//True如果行前空格數少於tabWidth，補齊空格數,False如果在文字前tab同true，如果在行首tab，則直接增加tabwidth個空格
	setTabWidth(4);//\t寬度設為四個空格

	setWhitespaceVisibility(QsciScintilla::WsInvisible);//此時空格為點，\t為箭頭
	setWhitespaceSize(0);//空格點大小
	setWhitespaceForegroundColor(QColor(85, 85, 85));

	copyAvailable(true);//允許自動複製選中範圍

	//selection color
	setSelectionBackgroundColor(QColor(38, 79, 120));//選中文本背景色
	setSelectionForegroundColor(Qt::white);//選中文本前景色

	setIndicatorDrawUnder(true);
	setIndicatorHoverForegroundColor(QColor(17, 61, 111));
	setIndicatorHoverStyle(QsciScintilla::FullBoxIndicator);
	setIndicatorOutlineColor(QColor(104, 119, 135));

	setMatchedBraceBackgroundColor(QColor(17, 61, 111));//括號等選取顏色
	setMatchedBraceForegroundColor(QColor(180, 180, 177));
	setUnmatchedBraceBackgroundColor(QColor(17, 61, 111));
	setUnmatchedBraceForegroundColor(QColor(255, 0x80, 0x80));

	setHotspotUnderline(true);
	setHotspotWrap(true);
	setHotspotBackgroundColor(QColor(30, 30, 30));
	setHotspotForegroundColor(QColor(128, 128, 255));

	//行號顯示區域
	setMarginType(0, QsciScintilla::NumberMargin);//設置標號為0的頁邊顯示行號
	setMarginLineNumbers(0, true);
	QFontMetrics fontmetrics = QFontMetrics(font);
	setMarginWidth(0, fontmetrics.horizontalAdvance("00000"));//

	setMarginsFont(font);//設置頁邊字體
	setMarginOptions(QsciScintilla::MoSublineSelect);
	setMarginsBackgroundColor(QColor(30, 30, 30));
	setMarginsForegroundColor(QColor(43, 145, 175));

	//斷點設置區域
	setMarginType(1, QsciScintilla::SymbolMargin); //設置1號頁邊顯示符號
	SendScintilla(QsciScintilla::SCI_MARKERSETFORE, 0, QColor(30, 30, 30)); //置標記前景和背景標記
	SendScintilla(QsciScintilla::SCI_MARKERSETBACK, 0, QColor(197, 81, 89));

	SendScintilla(QsciScintilla::SCI_MARKERSETFORE, 1, QColor(30, 30, 30)); //置標記前景和背景標記
	SendScintilla(QsciScintilla::SCI_MARKERSETBACK, 1, QColor(238, 242, 116));

	SendScintilla(QsciScintilla::SCI_MARKERSETFORE, 2, QColor(30, 30, 30)); //置標記前景和背景標記
	SendScintilla(QsciScintilla::SCI_MARKERSETBACK, 2, QColor(255, 128, 128));

	SendScintilla(QsciScintilla::SCI_MARKERSETFORE, 3, QColor(30, 30, 30)); //置標記前景和背景標記
	SendScintilla(QsciScintilla::SCI_MARKERSETBACK, 3, QColor(30, 30, 30));
	//SendScintilla(QsciScintilla::SCI_SETMARGINMASKN, 0, 0x01); //1號頁邊顯示2號標記
	//SendScintilla(SCI_SETMARGINSENSITIVEN, 1, true); //設置1號頁邊可以點擊
	setMarginSensitivity(1, true);
	//connect(this, SIGNAL(marginClicked(int, int, Qt::KeyboardModifiers)), parent, SLOT(addMarker(int, int, Qt::KeyboardModifiers)));

	markerDefine(QsciScintilla::Circle, SYM_POINT);
	markerDefine(QsciScintilla::RightArrow, SYM_ARROW);
	markerDefine(QsciScintilla::Underline, SYM_TRIANGLE);
	markerDefine(QsciScintilla::Invisible, SYM_STEP);
	setMarginMarkerMask(1, S_BREAK | S_ARROW | S_ERRORMARK | S_STEPMARK);
	setMarginWidth(1, 20);


	//
	//connect(this, SIGNAL(modificationChanged(bool)), parent, SLOT(on_modificationChanged(bool)));
	//connect(this, SIGNAL(cursorPositionChanged(int, int)), parent, SLOT(on_cursorPositionChanged(int, int)));

	//sig selectionChanged()
	//sig userListActivated(int 	id,const QString& string)
	//sig cursorPositionChanged(	int 	line,int 	index)


	//indicatorClicked(int 	line,int 	index,Qt::KeyboardModifiers state)
	//indicatorReleased



#pragma region style
	QString style = R"(
		#widget{
			color: rgb(250, 250, 250);
			background-color: rgb(30, 30, 30);
		}

		QToolTip{border-style:none; background-color: rgb(57, 58, 60);color: rgb(208, 208, 208);}


		QScrollBar:vertical {
			background: rgb(46,46,46);
			max-width: 18px;
		}

		QScrollBar::handle:vertical {
			border: 5px solid rgb(46,46,46);
			background: rgb(71,71,71);
		}

		QScrollBar::handle:hover:vertical,
		QScrollBar::handle:pressed:vertical {
			background: rgb(153,153,153);
		}

		QScrollBar::sub-page:vertical {background: 444444;}
		QScrollBar::add-page:vertical {background: 5B5B5B;}
		QScrollBar::add-line:vertical {background: none;}
		QScrollBar::sub-line:vertical {background: none;}

		QScrollBar:horizontal {
			background: rgb(71,71,71);
			border: 5px solid rgb(46,46,46);
			max-height: 18px;
		}

		QScrollBar::handle:horizontal {
			border: 5px solid rgb(46,46,46);
			background: rgb(71,71,71);
		}

		QScrollBar::handle:hover:horizontal,
		QScrollBar::handle:pressed:horizontal { background: rgb(153,153,153);}

		QScrollBar::sub-page:horizontal {background: 444444;}
		QScrollBar::add-page:horizontal {background: 5B5B5B;}
		QScrollBar::add-line:horizontal {background: none;}
		QScrollBar::sub-line:horizontal {background: none;}

		QListWidget{
			color: rgb(0, 0, 0);
			background-color: rgb(250, 250, 250);
		}

        QListWidget::item{
			color: rgb(0, 0, 0);
			background-color: rgb(250, 250, 250);
		}

		QListWidget::item:selected{ 
			color: rgb(0, 0, 0);
			background-color: rgb(0, 120, 215);
		}
	)";
	setAttribute(Qt::WA_StyledBackground);
	setStyleSheet(style);
#pragma endregion
}

void CodeEditor::keyPressEvent(QKeyEvent* e)
{
	if (e->modifiers() == Qt::ControlModifier)
	{
		//when press CTRL + / then lua comment or uncomment
		switch (e->key())
		{
		case Qt::Key_Slash:
		{
			commentSwitch();
			break;
		}
		case Qt::Key_F:
		{
			findReplace();
			return;
		}
		case Qt::Key_O:
		{
			foldAll(true);
			return;
		}
		case Qt::Key_M:
		{
			clearFolds();
			return;
		}
		case Qt::Key_G:
		{
			jumpToLineDialog();
			return;
		}
		}
	}

	return QsciScintilla::keyPressEvent(e);
}

void CodeEditor::findReplace()
{
	ReplaceDialog* replaceDialog = new ReplaceDialog(this);
	replaceDialog->setModal(false);
	replaceDialog->show();
}

void CodeEditor::commentSwitch()
{
	//the plaintext from this widget
	int selectionEnd = SendScintilla(QsciScintilla::SCI_GETSELECTIONEND);
	int selectionStart = SendScintilla(QsciScintilla::SCI_GETSELECTIONSTART);
	const QString str(selectedText());
	do
	{
		if (str.simplified().isEmpty())
		{
			int liner = NULL, index = NULL;
			getCursorPosition(&liner, &index);//記錄光標位置

			QString linetext(text(liner));
			if (linetext.simplified().indexOf("//") == 0 || linetext.simplified().indexOf("/*") == 0)
			{
				int indexd = linetext.indexOf("//");
				if (indexd == -1)
				{
					indexd = linetext.indexOf("/*");
				}
				//remove first "--"
				linetext.remove(indexd, 2);
				setSelection(liner, 0, liner, linetext.length() + 2);//少了之前去除的 -- 長度加回來
				replaceSelectedText(linetext);
			}
			else
			{
				setSelection(liner, 0, liner, linetext.length());
				const QString tmp("//" + linetext);
				replaceSelectedText(tmp);
			}
			setCursorPosition(liner, index);
			return;
		}
	} while (false);
	if ((selectionEnd > selectionStart))
	{
		bool isCRLF = false;
		QStringList v;
		if (str.endsWith("\r\n"))
		{
			v = str.split("\r\n"); //按行分割
			isCRLF = true;
		}
		else
			v = str.split("\n"); //按行分割

		int row_num = v.size();
		if (row_num == 0) return;
		//search all comment or all uncomment
		//and finally add back to widget by useing setText(str);
		bool allcomment = false;
		QString retstring("");
		if (v.at(0).simplified().indexOf("//") == -1 && v.at(0).simplified().indexOf("/*") == -1)
		{
			allcomment = true;
		}
		for (int i = 0; i < row_num; ++i)
		{
			//if allcomment is true even if the content already has comment mark still add additional comment
			//else if allcomment is false remove first command mark for each line if it has
			if (allcomment)
			{
				const QString tmp("//" + v.at(i));
				v[i] = tmp;
			}
			else
			{
				if (v.at(i).simplified().indexOf("//") == 0 || v.at(i).simplified().indexOf("/*") == 0)
				{
					int index = v.at(i).indexOf("//");
					if (index == -1)
					{
						index = v.at(i).indexOf("/*");
					}
					//remove first "--"
					Q_UNUSED(v[i].remove(index, 2));
				}
			}
		}
		if (isCRLF)
			retstring = (v.join("\r\n"));
		else
			retstring = (v.join("\n"));

		if (!retstring.isEmpty())
			replaceSelectedText(retstring);
	}
}


void CodeEditor::jumpToLineDialog()
{
	QSharedPointer<int> pline(new int(1));
	JumpToLineDialog* dialog = new JumpToLineDialog(this, pline.get());
	if (dialog == nullptr)
		return;

	isDialogOpened = true;

	//connect if accept then jump to line
	connect(dialog, &JumpToLineDialog::accepted, [this, pline]()
		{
			int line = *pline.get();
			setCursorPosition(line - 1, 0);
			QString text = this->text(line - 1);
			setSelection(line - 1, 0, line - 1, text.length());
			ensureLineVisible(line - 1);
			isDialogOpened = false;
		});

	connect(dialog, &JumpToLineDialog::rejected, [this]()
		{
			isDialogOpened = false;
		});

	connect(dialog, &JumpToLineDialog::close, [this]()
		{
			isDialogOpened = false;
		});

	dialog->show();

}