/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2023 All Rights Reserved.
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
#include <QObject>
#include <atomic>
#include "parser.h"
#include "util.h"

constexpr long long DEFAULT_FUNCTION_TIMEOUT = 5000;

class Interpreter : public QObject, public Indexer
{
	Q_OBJECT
public:
	enum VarShareMode
	{
		kNotShare,
		kShare,
	};

public:
	explicit Interpreter(long long index);
	virtual ~Interpreter();

	bool __fastcall isRunning() const;

	void __fastcall preview(const QString& fileName);

	void __fastcall doString(QString content);

	void __fastcall doFileWithThread(long long beginLine, const QString& fileName);

	bool __fastcall doFile(long long beginLine, const QString& fileName, Interpreter* pinterpretter, Parser* pparser, bool issub, Parser::Mode mode);

signals:
	void finished();

public slots:
	void proc();
	void onRunString();

private:
	long long scriptCallBack(long long currentIndex, long long currentLine, const TokenMap& token);

private:

	template<typename Func>
	void __fastcall registerFunction(const QString functionName, Func fun);
	void __fastcall openLibs();

private:
	bool __fastcall checkBattleThenWait();
	bool __fastcall checkOnlineThenWait();

	bool __fastcall waitfor(long long timeout, std::function<bool()> exprfun);
	bool __fastcall checkString(const TokenMap& TK, long long idx, QString* ret);
	bool __fastcall checkInteger(const TokenMap& TK, long long idx, long long* ret);

	long long __fastcall checkJump(const TokenMap& TK, long long idx, bool expr, JumpBehavior behavior);
	bool __fastcall checkRange(const TokenMap& TK, long long idx, long long* min, long long* max);
	bool __fastcall checkRelationalOperator(const TokenMap& TK, long long idx, RESERVE* ret) const;

	void __fastcall logExport(long long currentIndex, long long currentLine, const QString& text, long long color = 0);

	void __fastcall errorExport(long long currentIndex, long long currentLine, long long level, const QString& text);

	void __fastcall setParentParser(Parser* pParser) { pParentParser_ = pParser; }
	void __fastcall setParentInterpreter(Interpreter* pInterpreter) { pParentInterpreter_ = pInterpreter; }

private: //註冊給Parser的函數
	//system
	long long run(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long dostr(long long currentIndex, long long currentLine, const TokenMap& TK);

	//check
	long long waitmap(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long waitdlg(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long waitsay(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long waititem(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long waitpet(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long waitpos(long long currentIndex, long long currentLine, const TokenMap& TK);
	//check-group
	long long waitteam(long long currentIndex, long long currentLine, const TokenMap& TK);


	//move
	long long setdir(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long walkpos(long long currentIndex, long long currentLine, const TokenMap& TK);

	//action
	long long useitem(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long dropitem(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long swapitem(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long playerrename(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long petrename(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long setpetstate(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long droppet(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long buy(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long sell(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long sellpet(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long make(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long cook(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long usemagic(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long pickitem(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long depositgold(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long withdrawgold(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long addpoint(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long learn(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long trade(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long doffstone(long long currentIndex, long long currentLine, const TokenMap& TK);

	long long recordequip(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long wearequip(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long unwearequip(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long petequip(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long petunequip(long long currentIndex, long long currentLine, const TokenMap& TK);

	long long depositpet(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long deposititem(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long withdrawpet(long long currentIndex, long long currentLine, const TokenMap& TK);
	long long withdrawitem(long long currentIndex, long long currentLine, const TokenMap& TK);

	long long mail(long long currentIndex, long long currentLine, const TokenMap& TK);

	//battle
	long long bh(long long currentIndex, long long currentLine, const TokenMap& TK);//atk
	long long bj(long long currentIndex, long long currentLine, const TokenMap& TK);//magic
	long long bp(long long currentIndex, long long currentLine, const TokenMap& TK);//skill
	long long bs(long long currentIndex, long long currentLine, const TokenMap& TK);//switch
	long long be(long long currentIndex, long long currentLine, const TokenMap& TK);//escape
	long long bd(long long currentIndex, long long currentLine, const TokenMap& TK);//defense
	long long bi(long long currentIndex, long long currentLine, const TokenMap& TK);//item
	long long bt(long long currentIndex, long long currentLine, const TokenMap& TK);//catch
	long long bn(long long currentIndex, long long currentLine, const TokenMap& TK);//nothing
	long long bw(long long currentIndex, long long currentLine, const TokenMap& TK);//petskill
	long long bwf(long long currentIndex, long long currentLine, const TokenMap& TK);//pet nothing
	long long bwait(long long currentIndex, long long currentLine, const TokenMap& TK);//wait
	long long bend(long long currentIndex, long long currentLine, const TokenMap& TK);//wait

private:
	enum
	{
		WARN_LEVEL = 0,
		ERROR_LEVEL = 1,
	};

	Parser* pParentParser_ = nullptr;
	Interpreter* pParentInterpreter_ = nullptr;

	QThread thread_;
	Parser parser_;
	long long beginLine_ = 0;

	std::atomic_bool isRunning_ = false;
	ParserCallBack pCallback = nullptr;
	QList<QSharedPointer<Interpreter>> subInterpreterList_;
	QFutureSynchronizer<bool> subThreadFutureSync_;
};