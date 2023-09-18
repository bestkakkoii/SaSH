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

#include "stdafx.h"
#include "parser2.h"
#include "interpreter.h"

Parser2::Parser2()
{

}

Parser2::~Parser2()
{

}

//讀取腳本文件並轉換成Tokens
bool Parser2::loadFile(const QString& fileName, QString* pcontent)
{
	util::ScopedFile f(fileName, QIODevice::ReadOnly | QIODevice::Text);
	if (!f.isOpen())
		return false;

	const QFileInfo fi(fileName);
	const QString suffix = "." + fi.suffix();

	QString c;
	if (suffix == util::SCRIPT_DEFAULT_SUFFIX)
	{
		QTextStream in(&f);
		in.setCodec(util::DEFAULT_CODEPAGE);
		c = in.readAll();
		c.replace("\r\n", "\n");
		isPrivate_ = false;
	}
#ifdef CRYPTO_H
	else if (suffix == util::SCRIPT_PRIVATE_SUFFIX_DEFAULT)
	{
		Crypto crypto;
		if (!crypto.decodeScript(fileName, c))
			return false;

		isPrivate_ = true;
	}
#endif
	else
	{
		return false;
	}

	if (pcontent != nullptr)
	{
		*pcontent = c;
	}

	bool bret = loadString(c);
	if (bret)
		scriptFileName_ = fileName;
	return bret;
}

//將腳本內容轉換成Tokens
bool Parser2::loadString(const QString& content)
{
	bool bret = Lexer::tokenized(&lexer_, content);

	if (bret)
	{
		tokens_ = lexer_.getTokenMaps();
		labels_ = lexer_.getLabelList();
		functionNodeList_ = lexer_.getFunctionNodeList();
		forNodeList_ = lexer_.getForNodeList();
	}

	return bret;
}

void Parser2::parse(Interpreter* pParentInterpreter)
{

}

void Parser2::processTokens()
{

}