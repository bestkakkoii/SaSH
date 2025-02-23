﻿#include "stdafx.h"
#include <codeeditor.h>
#include <QApplication>
#include <QTextCursor>
#include <QTextEdit>
#include <QFile>
#include <QKeyEvent>
#include <qdialogbuttonbox.h>


#include <gamedevice.h>

#ifdef _WIN64
#ifdef _DEBUG
#pragma comment(lib, "qscintilla2_qt6d.lib")
#else
#pragma comment(lib, "qscintilla2_qt6.lib")
#endif
#else
#ifdef _DEBUG
#pragma comment(lib, "qscintilla2_qt5d.lib")
#else
#pragma comment(lib, "qscintilla2_qt5.lib")
#endif
#endif

CodeEditor::CodeEditor(QWidget* parent)
	: QsciScintilla(parent)
	, Indexer(-1)

{
	setAttribute(Qt::WA_StyledBackground);
	SendScintilla(QsciScintilla::SCI_SETCODEPAGE, QsciScintilla::SC_CP_UTF8);//設置編碼為UTF-8
	setUtf8(true);//設置編碼為UTF-8
	setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

	//install font
	QFontDatabase::addApplicationFont("YaHei Consolas Hybrid 1.12.ttf");
	font_ = util::getFont();
	font_.setFamily("YaHei Consolas Hybrid");
	font_.setPointSize(11);
	setFont(font_);

	QStringList paths;
	util::searchFiles(util::applicationDirPath(), "highlight", ".lua", &paths, false);
	QHash<long long, QByteArray> keywords;
	if (!paths.isEmpty())
	{
		sol::state lua;
		lua.open_libraries(
			sol::lib::base,
			sol::lib::package,
			sol::lib::os,
			sol::lib::string,
			sol::lib::math,
			sol::lib::table,
			sol::lib::debug,
			sol::lib::utf8,
			sol::lib::coroutine,
			sol::lib::io
		);

		lua.safe_script_file(paths.front().toStdString());

		do
		{
			sol::table table = lua["Keywords"];
			if (!table.valid())
				break;

			static const QHash<QString, Highlighter::HighLightColor> colorkeys = {
				{ "pink", Highlighter::kPink },
				{ "yellow", Highlighter::kYellow },
				{ "lightgreen", Highlighter::kLightGreen },
				{ "bluegreen", Highlighter::kBlueGreen },
				{ "darkblue", Highlighter::kDarkBlue },
				{ "lightblue", Highlighter::kLightBlue },
				{ "darkorange", Highlighter::kDarkOrange },
				{ "purple", Highlighter::kPurple }
			};

			for (const std::pair< sol::object, sol::object>& it : table)
			{
				if (!it.first.is<std::string>() || !it.second.is<std::string>())
					continue;

				const std::string key = it.first.as<std::string>();
				if (key.empty())
					continue;

				const QString qkey = util::toQString(key);

				if (!colorkeys.contains(qkey))
					continue;

				const std::string value = it.second.as<std::string>();
				if (value.empty())
					continue;

				QString qvalue = util::toQString(value);
				qvalue.replace("\r\n", " ");
				qvalue.replace("\r", " ");
				qvalue.replace("\t", " ");
				qvalue.replace("\f", " ");
				qvalue.replace("\v", " ");
				qvalue.replace("\n", " ");
				qvalue = qvalue.simplified();

				const Highlighter::HighLightColor color = colorkeys.value(qkey);

				keywords.insert(color, qvalue.toUtf8());
			}

		} while (false);
	}

	textLexer_ = new Highlighter(keywords, this);
	textLexer_->setDefaultFont(font_);
	apis_ = new QsciAPIs(textLexer_);
	setLexer(textLexer_);

	//代碼提示autoCompletion
	paths.clear();
	util::searchFiles(util::applicationDirPath(), "completion_api", ".txt", &paths, false);

	if (!paths.isEmpty())
	{
		QString content;
		if (util::readFile(paths.front(), &content) && !content.isEmpty())
		{
			QStringList list = content.split("\n");
			for (const auto& it : list)
			{
				apis_->add(it.simplified());
			}
			apis_->prepare();
		}
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
	setCallTipsStyle(QsciScintilla::CallTipsNoContext);
	setCallTipsPosition(QsciScintilla::CallTipsBelowText);
	setCallTipsBackgroundColor(QColor(30, 30, 30));

	setAnnotationDisplay(QsciScintilla::AnnotationBoxed);//註釋樣式

	//行號提示
	setFont(font_);//設置文本字體
	setWrapMode(QsciScintilla::WrapWord); //文本自動換行模式
	setWrapVisualFlags(QsciScintilla::WrapFlagByText);

	setIndentationsUseTabs(false);//false用表示用空格代替\t
	setIndentationGuides(true);//用tab鍵縮進時，在縮進位置上顯示一個豎點線，縮進有效，在字符串後加空格不顯示
	setIndentationWidth(0);//如果在行首部空格位置tab，縮進的寬度字符數，並且不會轉換為空格

	// 折疊標簽樣式
	setFolding(QsciScintilla::BoxedTreeFoldStyle, 2);//折疊樣式
	setFoldMarginColors(QColor(61, 61, 61), QColor(31, 31, 31));//折疊欄顏色

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
	QFont font = font_;
	font.setPointSize(12);
	QFontMetrics fontmetrics(font);
	setMarginWidth(0, fontmetrics.horizontalAdvance("00000000"));

	setMarginsFont(font);//設置頁邊字體
	setMarginOptions(QsciScintilla::MoSublineSelect);
	setMarginsBackgroundColor(QColor(30, 30, 30));
	setMarginsForegroundColor(QColor(43, 145, 175)/**/);

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
	setMarginWidth(1, 16);

	connect(&findAndReplaceForm_, &FindAndReplaceForm::findNexted, this, &CodeEditor::onFindNext);
	connect(&findAndReplaceForm_, &FindAndReplaceForm::findFirsted, this, &CodeEditor::onFindFirst);
	connect(&findAndReplaceForm_, &FindAndReplaceForm::replaceAlled, this, &CodeEditor::onReplaceAll);
	connect(&findAndReplaceForm_, &FindAndReplaceForm::findAlled, this, &CodeEditor::onFindAll);
	findAndReplaceForm_.hide();

	//
	//connect(this, SIGNAL(modificationChanged(bool)), parent, SLOT(on_modificationChanged(bool)));
	//connect(this, SIGNAL(cursorPositionChanged(int, int)), parent, SLOT(on_cursorPositionChanged(int, int)));

	//sig selectionChanged()
	//sig userListActivated(int 	id,const QString& string)
	//sig cursorPositionChanged(	int 	line,int 	index)


	//indicatorClicked(int 	line,int 	index,Qt::KeyboardModifiers state)
	//indicatorReleased

#pragma region style
	//QString style = R"(
	//	#widget{
	//		color: rgb(250, 250, 250);
	//		background-color: rgb(30, 30, 30);
	//	}

	//	QToolTip{border-style:none; background-color: rgb(57, 58, 60);color: rgb(208, 208, 208);}

	//	QScrollBar:vertical {
	//		background: rgb(46,46,46);
	//		max-width: 18px;
	//	}

	//	QScrollBar::handle:vertical {
	//		border: 5px solid rgb(46,46,46);
	//		background: rgb(71,71,71);
	//	}

	//	QScrollBar::handle:hover:vertical,
	//	QScrollBar::handle:pressed:vertical {
	//		background: rgb(153,153,153);
	//	}

	//	QScrollBar::sub-page:vertical {background: 444444;}
	//	QScrollBar::add-page:vertical {background: 5B5B5B;}
	//	QScrollBar::add-line:vertical {background: none;}
	//	QScrollBar::sub-line:vertical {background: none;}

	//	QScrollBar:horizontal {
	//		background: rgb(71,71,71);
	//		border: 5px solid rgb(46,46,46);
	//		max-height: 18px;
	//	}

	//	QScrollBar::handle:horizontal {
	//		border: 5px solid rgb(46,46,46);
	//		background: rgb(71,71,71);
	//	}

	//	QScrollBar::handle:hover:horizontal,
	//	QScrollBar::handle:pressed:horizontal { background: rgb(153,153,153);}

	//	QScrollBar::sub-page:horizontal {background: 444444;}
	//	QScrollBar::add-page:horizontal {background: 5B5B5B;}
	//	QScrollBar::add-line:horizontal {background: none;}
	//	QScrollBar::sub-line:horizontal {background: none;}

	//	QListWidget{
	//		color: rgb(0, 0, 0);
	//		background-color: rgb(250, 250, 250);
	//	}

 //       QListWidget::item{
	//		color: rgb(0, 0, 0);
	//		background-color: rgb(250, 250, 250);
	//	}

	//	QListWidget::item:selected{ 
	//		color: rgb(0, 0, 0);
	//		background-color: rgb(0, 120, 215);
	//	}
	//)";
	setAttribute(Qt::WA_StyledBackground);
	//setStyleSheet(style);
#pragma endregion
}

void CodeEditor::onFindAll(const QString& expr, bool re, bool cs, bool wo, bool wrap, bool forward, int line, int index, bool show, bool posix)
{
	setUpdatesEnabled(false);
	//move cursor to start
	setCursorPosition(0, 0);

	if (!findFirst(expr, re, cs, wo, wrap, forward, line, index, show, posix))
		return;

	//line, text|index
	QMap<long long, QVariantList> resultLineTexts;

	int currentLine = -1;
	int currentIndex = -1;
	getCursorPosition(&currentLine, &currentIndex);
	if (currentLine >= 0 && currentIndex >= 0)
	{
		resultLineTexts.insert(currentLine, QVariantList{ text(currentLine), currentIndex });
	}

	for (;;)
	{
		if (!findNext())
			break;

		int line = -1;
		int index = -1;
		getCursorPosition(&line, &index);
		if (line >= 0 && index >= 0)
		{
			resultLineTexts.insert(line, QVariantList{ text(line).simplified() , index - expr.size() });
		}
		QCoreApplication::processEvents();
	}

	setUpdatesEnabled(true);

	emit findAllFinished(expr, QVariant::fromValue(resultLineTexts));
}

void CodeEditor::onFindFirst(const QString& expr, bool re, bool cs, bool wo, bool wrap, bool forward, int line, int index, bool show, bool posix)
{
	findFirst(expr, re, cs, wo, wrap, forward, line, index, show, posix);
}

void CodeEditor::onFindNext()
{
	findNext();
}

void CodeEditor::onReplaceAll(const QString& replacetext, const QString& expr, bool re, bool cs, bool wo, bool wrap, bool forward, int line, int index, bool show, bool posix)
{
	setUpdatesEnabled(false);
	if (!findFirst(expr, re, cs, wo, wrap, forward, line, index, show, posix))
		return;

	for (;;)
	{
		replace(replacetext);
		if (!findNext())
			break;
		QCoreApplication::processEvents();
	}

	setUpdatesEnabled(true);
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
			//get selected text
			QString selectedText = this->selectedText();
			if (!selectedText.isEmpty())
				findAndReplaceForm_.setSearchText(selectedText);

			findAndReplaceForm_.hide();
			findAndReplaceForm_.show();
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
		default:
			break;
		}
	}
	//ctrl + shift
	else if (e->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))
	{
		switch (e->key())
		{
		case Qt::Key_F:
		{
			//get selected text
			QString selectedText = this->selectedText();
			if (!selectedText.isEmpty())
				findAndReplaceForm_.setSearchText(selectedText);

			findAndReplaceForm_.hide();
			findAndReplaceForm_.show();
			return;
		}
		default:
			break;
		}
	}

	return QsciScintilla::keyPressEvent(e);
}

void CodeEditor::dropEvent(QDropEvent* e)
{
	QsciScintilla::dropEvent(e);

	if (!e->mimeData()->hasUrls())
		return;

	//只取締一個
	QString path = e->mimeData()->urls().value(0).toLocalFile();

	if (path.isEmpty())
		return;

	QFileInfo info(path);
	QString suffix = info.suffix();
	if (suffix != "txt")
		return;

	GameDevice& gamedevice = GameDevice::getInstance(getIndex());
	if (gamedevice.IS_SCRIPT_FLAG.get())
		return;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(getIndex());
	emit signalDispatcher.loadFileToTable(path);

	gamedevice.currentScriptFileName.set(path);
}

void CodeEditor::commentSwitch()
{
	//the plaintext from this widget
	long long selectionEnd = SendScintilla(QsciScintilla::SCI_GETSELECTIONEND);
	long long selectionStart = SendScintilla(QsciScintilla::SCI_GETSELECTIONSTART);
	const QString str(selectedText());

	auto process = [this](const QString& str, long long selectionStart, long long selectionEnd,
		const QString& commentStr, const QString& areaCommentStrBegin)
		{
			do
			{
				if (str.simplified().isEmpty())
				{
					int liner = NULL, index = NULL;
					getCursorPosition(&liner, &index);//記錄光標位置

					QString linetext(text(liner));
					if (linetext.simplified().indexOf(commentStr) == 0 || linetext.simplified().indexOf(areaCommentStrBegin) == 0)
					{
						long long indexd = linetext.indexOf(commentStr);
						if (indexd == -1)
						{
							indexd = linetext.indexOf(areaCommentStrBegin);
						}
						//remove first "--"
						linetext.remove(indexd, 2);
						setSelection(liner, 0, liner, linetext.length() + 2);//少了之前去除的 -- 長度加回來
						replaceSelectedText(linetext);
					}
					else
					{
						setSelection(liner, 0, liner, linetext.length());
						const QString tmp(commentStr + linetext);
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

				long long row_num = v.size();
				if (row_num == 0)
					return;

				//search all comment or all uncomment
				//and finally add back to widget by useing setText(str);
				bool allcomment = false;
				QString retstring("");
				if (v.value(0).simplified().indexOf(commentStr) == -1 && v.value(0).simplified().indexOf(areaCommentStrBegin) == -1)
				{
					allcomment = true;
				}
				for (long long i = 0; i < row_num; ++i)
				{
					//if allcomment is true even if the content already has comment mark still add additional comment
					//else if allcomment is false remove first command mark for each line if it has
					if (allcomment)
					{
						const QString tmp(commentStr + v.value(i));
						v[i] = tmp;
					}
					else
					{
						if (v.value(i).simplified().indexOf(commentStr) == 0 || v.value(i).simplified().indexOf(areaCommentStrBegin) == 0)
						{
							long long index = v.value(i).indexOf(commentStr);
							if (index == -1)
							{
								index = v.value(i).indexOf(areaCommentStrBegin);
							}
							//remove first "--"
							std::ignore = v[i].remove(index, 2);
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
		};

	if (".lua" == suffix_)
	{
		process(str, selectionStart, selectionEnd, "--", "--[[");
	}
	else
	{
		process(str, selectionStart, selectionEnd, "//", "/*");
	}
}

void CodeEditor::jumpToLineDialog()
{
	long long* pline(q_check_ptr(new long long(1)));
	sash_assume(pline != nullptr);
	if (nullptr == pline)
		return;

	JumpToLineDialog* dialog = q_check_ptr(new JumpToLineDialog(this, pline));
	sash_assume(dialog != nullptr);
	if (dialog == nullptr)
		return;

	isDialogOpened_ = true;

	//connect if accept then jump to line
	connect(dialog, &JumpToLineDialog::accepted, [this, pline]()
		{
			long long line = *pline;
			setCursorPosition(line - 1, 0);
			QString text = this->text(line - 1);
			setSelection(line - 1, 0, line - 1, text.length());
			ensureLineVisible(line - 1);
			isDialogOpened_ = false;
			delete pline;
		});

	connect(dialog, &JumpToLineDialog::rejected, [this]()
		{
			isDialogOpened_ = false;
		});

	connect(dialog, &JumpToLineDialog::close, [this]()
		{
			isDialogOpened_ = false;
		});

	dialog->show();

}