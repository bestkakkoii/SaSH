#include "stdafx.h"
#include "parser.h"

#include "signaldispatcher.h"
#include "injector.h"
#include "interpreter.h"

//根據token解釋腳本
void Parser::parse(int line)
{
	lineNumber_ = line; //設置當前行號
	callStack_.clear(); //清空調用棧
	jmpStack_.clear();  //清空跳轉棧

	if (tokens_.isEmpty())
		return;

	variables_.reset(new VariantSafeHash());
	if (variables_.isNull())
		return;

	processTokens();
}

//"調用" 傳參數最小佔位
constexpr int kCallPlaceHoldSize = 2;
//"格式化" 最小佔位
constexpr int kFormatPlaceHoldSize = 3;

//變量運算
template<typename T>
T calc(const QVariant& a, const QVariant& b, RESERVE operatorType)
{
	if (operatorType == TK_ADD)
	{
		return a.value<T>() + b.value<T>();
	}
	else if (operatorType == TK_SUB)
	{
		return a.value<T>() - b.value<T>();
	}
	else if (operatorType == TK_MUL)
	{
		return a.value<T>() * b.value<T>();
	}
	else if (operatorType == TK_DIV)
	{
		return a.value<T>() / b.value<T>();
	}
	else if (operatorType == TK_INC)
	{
		if constexpr (std::is_integral<T>::value)
			return a.value<T>() + 1;
		else
			return a.value<T>() + 1.0;
	}
	else if (operatorType == TK_DEC)
	{
		if constexpr (std::is_integral<T>::value)
			return a.value<T>() - 1;
		else
			return a.value<T>() - 1.0;
	}

	Q_UNREACHABLE();
}

bool Parser::checkString(const TokenMap& TK, int idx, QString* ret)
{
	if (!TK.contains(idx))
		return false;
	if (ret == nullptr)
		return false;

	RESERVE type = TK.value(idx).type;
	QVariant var = TK.value(idx).data;
	QHash<QString, QVariant> args = getLabelVars();
	if (!var.isValid())
		return false;
	if (type == TK_REF)
	{
		*ret = getVar<QString>(var.toString());
	}
	else if (type == TK_STRING || type == TK_CMD)
	{
		//檢查是否為區域變量
		QString name = var.toString();
		if (args.contains(name))
		{
			QVariant::Type vtype = args.value(name).type();
			if (vtype == QVariant::Int || vtype == QVariant::String)
				*ret = args.value(name).toString();
			else if (vtype == QVariant::Double)
				*ret = QString::number(args.value(name).toDouble(), 'f', 8);
			else
				return false;
		}
		else
			*ret = var.toString();
	}
	else
		return false;

	return true;
}

bool Parser::checkInt(const TokenMap& TK, int idx, int* ret)
{
	if (!TK.contains(idx))
		return false;
	if (ret == nullptr)
		return false;

	RESERVE type = TK.value(idx).type;
	QVariant var = TK.value(idx).data;
	QHash<QString, QVariant> args = getLabelVars();
	if (!var.isValid())
		return false;

	if (type == TK_REF)
	{
		*ret = getVar<int>(var.toString());
	}
	else if (type == TK_INT)
	{
		bool ok = false;
		int value = var.toInt(&ok);
		if (!ok)
			return false;
		*ret = value;
	}
	else if (type == TK_STRING)
	{
		//檢查是否為區域變量
		QString name = var.toString();
		if (args.contains(name) && args.value(name).type() == QVariant::Int)
		{
			*ret = args.value(name).toInt();
		}
		else if (args.contains(name) && args.value(name).type() == QVariant::String)
		{
			bool ok;
			int value = args.value(name).toInt(&ok);
			if (ok)
				*ret = value;
			else
				return false;
		}
		else
			return false;
	}
	else
		return false;

	return true;
}

//行跳轉
void Parser::jump(int line, bool noStack)
{
	if (!noStack)
		jmpStack_.push(lineNumber_ + 1);
	lineNumber_ += line;
}

//指定行跳轉
void Parser::jumpto(int line, bool noStack)
{
	if (line - 1 < 0)
		line = 1;
	if (!noStack)
		jmpStack_.push(lineNumber_ + 1);
	lineNumber_ = line - 1;
}

//標記跳轉
bool Parser::jump(const QString& name, bool noStack)
{
	int jumpLine = matchLineFromLabel(name);
	if (jumpLine == -1)
	{
		handleError(kLabelError);
		return false;
	}

	if (!noStack)
		jmpStack_.push(lineNumber_ + 1);

	int jumpLineCount = jumpLine - lineNumber_;
	jump(jumpLineCount, true);
	return true;
}

//處理所有的token
void Parser::processTokens()
{
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	Injector& injector = Injector::getInstance();

	if (mode_ == kSync)
	{
		emit signalDispatcher.addErrorMarker(-1, false);
		emit signalDispatcher.addForwardMarker(-1, false);
		emit signalDispatcher.addStepMarker(-1, false);
		generateStackInfo(0);
		generateStackInfo(1);
	}

	int oldlineNumber = lineNumber_;
	for (;;)
	{
		if (isEmpty())
			break;


		RESERVE currentType = currentLineTokens_.value(0).type;

		if (currentType != RESERVE::TK_WHITESPACE && currentType != RESERVE::TK_SPACE)
		{
			if (mode_ == Mode::kSync)
				emit signalDispatcher.scriptLabelRowTextChanged2(oldlineNumber, NULL, false);

			QThread::yieldCurrentThread();
			int extraDelay = injector.getValueHash(util::kScriptSpeedValue);
			if (extraDelay > 1000)
			{
				int i = 0;
				for (i = 0; i < extraDelay / 1000; ++i)
					QThread::msleep(1000);
				QThread::msleep(extraDelay % 1000);
			}
			else if (extraDelay > 0)
			{
				QThread::msleep(extraDelay);
			}

			QThread::msleep(1);
		}

		oldlineNumber = lineNumber_;

		currentLineTokens_ = tokens_.value(lineNumber_);
		if (callBack_)
		{
			int status = callBack_(lineNumber_, currentLineTokens_);
			if (status == kStop)
				break;
		}

		qDebug() << "line:" << lineNumber_ << "tokens:" << currentLineTokens_.value(0).raw;
		RESERVE type = getType();

		try
		{
			switch (type)
			{
			case TK_END:
			{
				lastError_ = kNoError;
				processEnd();
				return;
			}
			case TK_CMD:
			{
				int ret = processCommand();
				if (ret == kHasJump)
				{
					generateStackInfo(1);
					continue;
				}
				else if (ret == kError)
				{
					handleError(ret);
					return;
				}
				else if (ret == kArgError)
				{
					handleError(ret);
				}
				break;
			}
			case TK_VARDECL:
			case TK_VARFREE:
			case TK_VARCLR:
				processVariable(type);
				break;
			case TK_MULTIVAR:
				processMultiVariable();
				break;
			case TK_FORMAT:
				processFormation();
				break;
			case TK_RND:
				processRandom();
				break;
			case TK_CALL:
			{
				if (processCall())
				{
					generateStackInfo(0);
					continue;
				}
				break;
			}
			case TK_GOTO:
			{
				if (processGoto())
				{
					generateStackInfo(1);
					continue;
				}
				break;
			}
			case TK_JMP:
			{
				if (processJump())
				{
					generateStackInfo(1);
					continue;
				}
				break;
			}
			case TK_RETURN:
				processReturn();
				generateStackInfo(0);
				continue;
			case TK_BAK:
				processBack();
				generateStackInfo(1);
				continue;
			case TK_LABEL:
			{
				processLabel();
				break;
			}

			case TK_WHITESPACE:
				break;
			default:
				qDebug() << "Unexpected token type:" << type;
				break;
			}
		}
		catch (...)
		{
			lastError_ = kException;
			break;
		}

		if (isStop_.load(std::memory_order_acquire))
			break;

		//導出變量訊息
		if (mode_ == kSync)
		{
			emit signalDispatcher.globalVarInfoImport(variables_->toHash());
			if (!labalVarStack_.isEmpty())
				emit signalDispatcher.localVarInfoImport(labalVarStack_.top());//導出局變量訊息
			else
				emit signalDispatcher.localVarInfoImport(QHash<QString, QVariant>());
		}

		//移動至下一行
		next();
	}

	processEnd();
}

//處理"標記"，檢查是否有聲明局變量
void Parser::processLabel()
{
	QVariantList args = getArgsRef();
	QHash<QString, QVariant> labelVars;
	for (int i = kCallPlaceHoldSize; i < currentLineTokens_.size(); ++i)
	{
		Token token = currentLineTokens_.value(i);
		if (token.type == TK_LABELVAR)
		{
			QString labelName = token.data.toString();
			if (labelName.isEmpty())
				continue;

			if (!args.isEmpty() && (args.size() > (i - kCallPlaceHoldSize)) && (args.at(i - kCallPlaceHoldSize).isValid()))
				labelVars.insert(labelName, args.at(i - kCallPlaceHoldSize));
		}
	}

	if (!labelVars.isEmpty())
		labalVarStack_.push(labelVars);
}

//處理"結束"
void Parser::processEnd()
{
	callStack_.clear();
	jmpStack_.clear();
	callArgsStack_.clear();
	labalVarStack_.clear();
}

//處理所有核心命令之外的所有命令
int Parser::processCommand()
{
	TokenMap tokens = getCurrentTokens();
	Token commandToken = tokens.value(0);
	QString commandName = commandToken.data.toString();
	int status = kNoChange;
	if (commandRegistry_.contains(commandName))
	{
		CommandRegistry function = commandRegistry_.value(commandName);
		if (function == nullptr)
		{
			qDebug() << "Command not registered:" << commandName;
			return status;
		}

		status = function(lineNumber_, tokens);
	}
	else
	{
		qDebug() << "Command not found:" << commandName;
	}
	return status;
}

//處理"變量"
void Parser::processVariable(RESERVE type)
{

	switch (type)
	{
	case TK_VARDECL:
	{
		//取第一個參數
		QString varName = getToken<QString>(1);
		if (varName.isEmpty())
			break;

		//取第二個參數
		QVariant varValue = getToken<QVariant>(2);
		if (!varValue.isValid())
			break;

		//取區域變量表
		QHash<QString, QVariant> args = getLabelVars();

		//檢查第二參數是否為邏輯運算符
		RESERVE op = getTokenType(2);
		if (!operatorTypes.contains(op))
		{
			SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
			//如果不是運算符，檢查是否為字符串
			bool pass = false;
			if (varValue.type() == QVariant::Type::String)
			{
				//檢查是否為引用變量
				QString valueStr = varValue.toString();
				if (op == TK_REF)
				{
					valueStr = valueStr.mid(1);
					if (variables_->contains(valueStr))
						varValue = variables_->value(valueStr);
					else
						return;
				}
				else
				{
					//檢查是否為字符串的區域變量
					if (args.contains(valueStr) && args.value(valueStr).type() == QVariant::String)
					{
						varValue = args.value(valueStr).toString();
					}
					//檢查是否為浮點數的區域變量
					else if (args.contains(valueStr) && args.value(valueStr).type() == QVariant::Double)
					{
						bool ok;
						double value = args.value(valueStr).toDouble(&ok);
						if (ok)
						{
							varValue = value;
						}
						else
							break;
					}
					//檢查是否為整數的區域變量
					else if (args.contains(valueStr) && args.value(valueStr).type() == QVariant::Int)
					{
						bool ok;
						int value = args.value(valueStr).toInt(&ok);
						if (ok)
						{
							varValue = value;
						}
						else
							break;
					}
					else
					{
						//檢查是否使用?讓使用者輸入
						QString msg = getToken<QString>(3);
						if (valueStr.startsWith(kFuzzyPrefix + QString("2")))//整數輸入框
						{
							varValue.clear();
							emit signalDispatcher.inputBoxShow(msg, QInputDialog::IntInput, &varValue);
							if (!varValue.isValid())
								break;
							varValue = varValue.toInt();
						}
						else if (valueStr.startsWith(kFuzzyPrefix + QString("3"))) //雙精度浮點數輸入框
						{
							varValue.clear();
							emit signalDispatcher.inputBoxShow(msg, QInputDialog::DoubleInput, &varValue);
							if (!varValue.isValid())
								break;
							varValue = varValue.toDouble();
						}
						else if (valueStr.startsWith(kFuzzyPrefix) && valueStr.endsWith(kFuzzyPrefix) || valueStr.startsWith(kFuzzyPrefix + QString("1")))// 字串輸入框
						{
							varValue.clear();
							emit signalDispatcher.inputBoxShow(msg, QInputDialog::TextInput, &varValue);
							if (!varValue.isValid())
								break;
							varValue = varValue.toString();
						}
					}
				}

				if (processGetSystemVarValue(varName, valueStr, varValue))
				{
					variables_->insert(varName, varValue);
					break;
				}
			}

			//插入全局變量表
			variables_->insert(varName, varValue);
			break;
		}

		//下面是變量比較數值，先檢查變量是否存在
		if (!variables_->contains(varName))
		{
			if (op != TK_DEC && op != TK_INC)
				break;
			else
				variables_->insert(varName, 0);
		}


		if (op == TK_UNK)
			break;

		//取第三個參數
		varValue = getToken<QVariant>(3);
		//檢查第三個是否合法 ，如果不合法 且第二參數不是++或--，則跳出
		if (!varValue.isValid() && op != TK_DEC && op != TK_INC)
			break;

		//取第三個參數的類型
		RESERVE varValueType = getTokenType(3);

		//檢查是否為引用變量
		if (varValueType == TK_REF)
		{
			QString valueStr = varValue.toString().mid(1);
			if (variables_->contains(valueStr))
				varValue = variables_->value(valueStr);
		}
		else if (varValueType == TK_STRING)
		{
			QString valueStr = varValue.toString();
			//檢查是否為字符串的區域變量
			if (args.contains(valueStr) && args.value(valueStr).type() == QVariant::String)
			{
				varValue = args.value(valueStr).toString();
			}
			//檢查是否為浮點數的區域變量
			else if (args.contains(valueStr) && args.value(valueStr).type() == QVariant::Double)
			{
				bool ok;
				double value = args.value(valueStr).toDouble(&ok);
				if (ok)
				{
					varValue = value;
				}
				else
					break;
			}
			//檢查是否為整數的區域變量
			else if (args.contains(valueStr) && args.value(valueStr).type() == QVariant::Int)
			{
				bool ok;
				int value = args.value(valueStr).toInt(&ok);
				if (ok)
				{
					varValue = value;
				}
				else
					break;
			}
		}

		//將第一參數，要被運算的變量原數值取出
		QVariant& var = (*variables_)[varName];

		//開始計算新值
		variableCalculate(varName, op, &var, varValue);
		break;
	}
	case TK_VARFREE:
	{
		QString varName = getToken<QString>(1);
		if (varName.isEmpty())
			break;

		if (variables_->contains(varName))
			variables_->remove(varName);
		break;
	}
	case TK_VARCLR:
	{
		variables_->clear();
		break;
	}
	}
}

void Parser::processMultiVariable()
{
	QString varNameStr = getToken<QString>(0);
	QStringList varNames = varNameStr.split(util::rexComma, Qt::SkipEmptyParts);
	int varCount = varNames.count();

	//取區域變量表
	QHash<QString, QVariant> args = getLabelVars();


	for (int i = 0; i < varCount; ++i)
	{
		QString varName = varNames.at(i);
		if (varName.isEmpty())
		{
			variables_->insert(varName, 0);
			continue;
		}

		if (i + 1 >= currentLineTokens_.size())
		{
			variables_->insert(varName, 0);
			continue;
		}

		QVariant varValue = getToken<QVariant>(i + 1);
		//如果不合法 則跳出
		if (!varValue.isValid())
		{
			variables_->insert(varName, 0);
			continue;
		}

		RESERVE varValueType = getTokenType(i + 1);

		//檢查是否為引用變量
		if (varValueType == TK_REF)
		{
			QString valueStr = varValue.toString().mid(1);
			if (variables_->contains(valueStr))
				varValue = variables_->value(valueStr);
		}
		else if (varValueType == TK_STRING)
		{
			QString valueStr = varValue.toString();
			//檢查是否為字符串的區域變量
			if (args.contains(valueStr) && args.value(valueStr).type() == QVariant::String)
			{
				varValue = args.value(valueStr).toString();
			}
			//檢查是否為浮點數的區域變量
			else if (args.contains(valueStr) && args.value(valueStr).type() == QVariant::Double)
			{
				bool ok;
				double value = args.value(valueStr).toDouble(&ok);
				if (ok)
				{
					varValue = value;
				}
				else
				{
					variables_->insert(varName, 0);
					continue;
				}
			}
			//檢查是否為整數的區域變量
			else if (args.contains(valueStr) && args.value(valueStr).type() == QVariant::Int)
			{
				bool ok;
				int value = args.value(valueStr).toInt(&ok);
				if (ok)
				{
					varValue = value;
				}
				else
				{
					variables_->insert(varName, 0);
					continue;
				}
			}
		}

		variables_->insert(varName, varValue);
	}
}

//根據關鍵字取值保存到變量
bool Parser::processGetSystemVarValue(const QString& varName, QString& valueStr, QVariant& varValue)
{
	Injector& injector = Injector::getInstance();
	if (injector.server.isNull())
		return false;

	QString trimmedStr = valueStr.simplified().toLower();

	enum SystemVarName
	{
		kPlayerInfo,
		kMagicInfo,
		kSkillInfo,
		kPetInfo,
		kPetSkillInfo,
		kMapInfo,
		kItemInfo,
		kEquipInfo,
		kPetEquipInfo,
		kPartyInfo,
		kChatInfo,
		kDialogInfo,
		kPointInfo,
	};

	QHash<QString, SystemVarName> systemVarNameHash = {
		{ "player", kPlayerInfo },
		{ "magic", kMagicInfo },
		{ "skill", kSkillInfo },
		{ "pet", kPetInfo },
		{ "petskill", kPetSkillInfo },
		{ "map", kMapInfo },
		{ "item", kItemInfo },
		{ "equip", kEquipInfo },
		{ "petequip", kPetEquipInfo },
		{ "party", kPartyInfo },
		{ "chat", kChatInfo },
		{ "dialog", kDialogInfo },
		{ "point", kPointInfo }
	};

	if (!systemVarNameHash.contains(trimmedStr))
		return false;

	SystemVarName index = systemVarNameHash.value(trimmedStr);
	bool bret = false;
	switch (index)
	{
	case kPlayerInfo:
	{
		QString typeStr;
		checkString(currentLineTokens_, 3, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		util::SafeHash<QString, QVariant>& hashpc = injector.server->hashpc;
		if (!hashpc.contains(typeStr))
			break;

		varValue = hashpc.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	case kMagicInfo:
	{
		int magicIndex = -1;
		if (!checkInt(currentLineTokens_, 3, &magicIndex))
			break;

		QString typeStr;
		checkString(currentLineTokens_, 4, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		util::SafeHash<int, util::SafeHash<QString, QVariant>>& hashmagic = injector.server->hashmagic;
		if (!hashmagic.contains(magicIndex))
			break;

		util::SafeHash<QString, QVariant>& hash = hashmagic[magicIndex];
		if (!hash.contains(typeStr))
			break;

		varValue = hash.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	case kSkillInfo:
	{
		int skillIndex = -1;
		if (!checkInt(currentLineTokens_, 3, &skillIndex))
			break;

		QString typeStr;
		checkString(currentLineTokens_, 4, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		util::SafeHash<int, util::SafeHash<QString, QVariant>>& hashskill = injector.server->hashskill;
		if (!hashskill.contains(skillIndex))
			break;

		util::SafeHash<QString, QVariant>& hash = hashskill[skillIndex];
		if (!hash.contains(typeStr))
			break;

		varValue = hash.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	case kPetInfo:
	{
		int petIndex = -1;
		if (!checkInt(currentLineTokens_, 3, &petIndex))
			break;

		QString typeStr;
		checkString(currentLineTokens_, 4, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		util::SafeHash<int, util::SafeHash<QString, QVariant>>& hashpet = injector.server->hashpet;
		if (!hashpet.contains(petIndex))
			break;

		util::SafeHash<QString, QVariant>& hash = hashpet[petIndex];
		if (!hash.contains(typeStr))
			break;

		varValue = hash.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	case kPetSkillInfo:
	{
		int petIndex = -1;
		if (!checkInt(currentLineTokens_, 3, &petIndex))
			break;

		int skillIndex = -1;
		if (!checkInt(currentLineTokens_, 4, &skillIndex))
			break;

		QString typeStr;
		checkString(currentLineTokens_, 5, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		util::SafeHash<int, util::SafeHash<int, util::SafeHash<QString, QVariant>>>& hashpetskill = injector.server->hashpetskill;
		if (!hashpetskill.contains(petIndex))
			break;

		util::SafeHash<int, util::SafeHash<QString, QVariant>>& hash = hashpetskill[petIndex];
		if (!hash.contains(skillIndex))
			break;

		util::SafeHash<QString, QVariant>& hash2 = hash[skillIndex];
		if (!hash2.contains(typeStr))
			break;

		varValue = hash2.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	case kMapInfo:
	{
		QString typeStr;
		checkString(currentLineTokens_, 3, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		util::SafeHash<QString, QVariant>& hashmap = injector.server->hashmap;
		if (!hashmap.contains(typeStr))
			break;

		varValue = hashmap.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	case kItemInfo:
	{
		int itemIndex = -1;
		if (!checkInt(currentLineTokens_, 3, &itemIndex))
		{
			QString typeStr;
			if (checkString(currentLineTokens_, 3, &typeStr))
			{
				typeStr = typeStr.simplified().toLower();
				if (typeStr == "space")
				{
					QVector<int> itemIndexs;
					int size = 0;
					if (injector.server->getItemEmptySpotIndexs(&itemIndexs))
						size = itemIndexs.size();

					varValue = size;
					bret = varValue.isValid();
					break;
				}
				else
					break;
			}
		}

		QString typeStr;
		checkString(currentLineTokens_, 4, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		util::SafeHash<int, util::SafeHash<QString, QVariant>>& hashitem = injector.server->hashitem;
		if (!hashitem.contains(itemIndex))
			break;

		util::SafeHash<QString, QVariant>& hash = hashitem[itemIndex];
		if (!hash.contains(typeStr))
			break;

		varValue = hash.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	case kEquipInfo:
	{
		int itemIndex = -1;
		if (!checkInt(currentLineTokens_, 3, &itemIndex))
			break;

		QString typeStr;
		checkString(currentLineTokens_, 4, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		util::SafeHash<int, util::SafeHash<QString, QVariant>>& hashitem = injector.server->hashequip;
		if (!hashitem.contains(itemIndex))
			break;

		util::SafeHash<QString, QVariant>& hash = hashitem[itemIndex];
		if (!hash.contains(typeStr))
			break;

		varValue = hash.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	case kPetEquipInfo:
	{
		int petIndex = -1;
		if (!checkInt(currentLineTokens_, 3, &petIndex))
			break;

		int itemIndex = -1;
		if (!checkInt(currentLineTokens_, 4, &itemIndex))
			break;

		QString typeStr;
		checkString(currentLineTokens_, 5, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		util::SafeHash<int, util::SafeHash<int, util::SafeHash<QString, QVariant>>>& hashpetequip = injector.server->hashpetequip;
		if (!hashpetequip.contains(petIndex))
			break;

		util::SafeHash<int, util::SafeHash<QString, QVariant>>& hash = hashpetequip[petIndex];
		if (!hash.contains(itemIndex))
			break;

		util::SafeHash<QString, QVariant>& hash2 = hash[itemIndex];
		if (!hash2.contains(typeStr))
			break;

		varValue = hash2.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	case kPartyInfo:
	{
		int partyIndex = -1;
		if (!checkInt(currentLineTokens_, 3, &partyIndex))
			break;

		QString typeStr;
		checkString(currentLineTokens_, 4, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;

		util::SafeHash<int, util::SafeHash<QString, QVariant>>& hashparty = injector.server->hashparty;
		if (!hashparty.contains(partyIndex))
			break;

		util::SafeHash<QString, QVariant>& hash = hashparty[partyIndex];
		if (!hash.contains(typeStr))
			break;

		varValue = hash.value(typeStr);
		bret = varValue.isValid();
		break;
	}
	case kChatInfo:
	{
		int chatIndex = -1;
		if (!checkInt(currentLineTokens_, 3, &chatIndex))
			break;

		util::SafeHash<int, QVariant>& hashchat = injector.server->hashchat;
		if (!hashchat.contains(chatIndex))
			break;

		varValue = hashchat.value(chatIndex);
		bret = varValue.isValid();
		break;
	}
	case kDialogInfo:
	{
		int dialogIndex = -1;
		if (!checkInt(currentLineTokens_, 3, &dialogIndex))
			break;

		util::SafeHash<int, QVariant>& hashdialog = injector.server->hashdialog;
		if (!hashdialog.contains(dialogIndex))
			break;

		varValue = hashdialog.value(dialogIndex);
		bret = varValue.isValid();
		break;
	}
	case kPointInfo:
	{
		QString typeStr;
		checkString(currentLineTokens_, 3, &typeStr);
		typeStr = typeStr.simplified().toLower();
		if (typeStr.isEmpty())
			break;


		injector.server->IS_WAITFOR_EXTRA_DIALOG_INFO_FLAG = true;
		injector.server->shopOk(2);

		QElapsedTimer timer; timer.start();
		for (;;)
		{
			if (timer.hasExpired(5000))
				break;

			if (!injector.server->IS_WAITFOR_EXTRA_DIALOG_INFO_FLAG)
				break;

			QThread::msleep(100);
		}
		//int rep = 0;   // 聲望
		//int ene = 0;   // 氣勢
		//int shl = 0;   // 貝殼
		//int vit = 0;   // 活力
		//int pts = 0;   // 積分
		//int vip = 0;   // 會員點
		if (typeStr == "exp")
		{
			varValue = injector.server->currencyData.expbufftime;
			bret = varValue.isValid();
		}
		else if (typeStr == "rep")
		{
			varValue = injector.server->currencyData.prestige;
			bret = varValue.isValid();
		}
		else if (typeStr == "ene")
		{
			varValue = injector.server->currencyData.energy;
			bret = varValue.isValid();
		}
		else if (typeStr == "shl")
		{
			varValue = injector.server->currencyData.shell;
			bret = varValue.isValid();
		}
		else if (typeStr == "vit")
		{
			varValue = injector.server->currencyData.vitality;
			bret = varValue.isValid();
		}
		else if (typeStr == "pts")
		{
			varValue = injector.server->currencyData.points;
			bret = varValue.isValid();
		}
		else if (typeStr == "vip")
		{
			varValue = injector.server->currencyData.VIPPoints;
			bret = varValue.isValid();
		}

		if (bret)
			injector.server->press(BUTTON_CANCEL);

		break;
	}
	default:
		break;
	}

	return bret;
}

//處理隨機數
void Parser::processRandom()
{
	do
	{
		QString varName = getToken<QString>(1);
		if (varName.isEmpty())
			break;

		int min = getToken<int>(2);
		int max = getToken<int>(3);

		if (min > 0 && max == 0)
		{
			variables_->insert(varName, QRandomGenerator::global()->bounded(0, min));
			break;
		}
		else if (min > max)
		{
			variables_->insert(varName, 0);
			break;
		}

		variables_->insert(varName, QRandomGenerator::global()->bounded(min, max));
	} while (false);
}

//處理"格式化"
void Parser::processFormation()
{

	auto formatTime = [](int seconds)->QString
	{
		int hours = seconds / 3600;
		int minutes = (seconds % 3600) / 60;
		int remainingSeconds = seconds % 60;

		return QString(QObject::tr("%1 hour %2 min %3 sec")).arg(hours).arg(minutes).arg(remainingSeconds);
	};

	do
	{
		QString varName = getToken<QString>(1);
		if (varName.isEmpty())
			break;

		QVariant varValue = getToken<QVariant>(2);
		if (!varValue.isValid())
			break;

		QString formatStr = varValue.toString();
		if (formatStr.isEmpty())
			break;

		QHash<QString, QVariant> args = getLabelVars();

		//查找字符串中包含 {:變數名} 全部替換成變數數值
		for (auto it = variables_->cbegin(); it != variables_->cend(); ++it)
		{
			QString key = QString("{:%1}").arg(it.key());
			QString keyWithTime = QString("{T:%1}").arg(it.key());
			if (formatStr.contains(key))
				formatStr.replace(key, it.value().toString());
			else if (formatStr.contains(keyWithTime))
			{
				int seconds = it.value().toInt();
				formatStr.replace(keyWithTime, formatTime(seconds));
			}
		}

		for (auto it = args.cbegin(); it != args.cend(); ++it)
		{
			QString key = QString("{L:%1}").arg(it.key());
			QString keyWithTime = QString("{LT:%1}").arg(it.key());
			if (formatStr.contains(key))
				formatStr.replace(key, it.value().toString());
			else if (formatStr.contains(keyWithTime))
			{
				int seconds = it.value().toInt();
				formatStr.replace(keyWithTime, formatTime(seconds));
			}
		}

		int argsize = tokens_.size();
		for (int i = kFormatPlaceHoldSize; i < argsize; ++i)
		{
			varValue = getToken<QVariant>(i);
			if (!varValue.isValid())
				continue;

			RESERVE type = getTokenType(i);
			if (type == TK_INT || type == TK_STRING)
			{
				if (type == TK_STRING)
				{
					QString valueStr = varValue.toString();
					if (args.contains(valueStr) && args.value(valueStr).type() == QVariant::String)
					{
						varValue = args.value(valueStr).toString();
					}
					//檢查是否為浮點數的區域變量
					else if (args.contains(valueStr) && args.value(valueStr).type() == QVariant::Double)
					{
						bool ok;
						double value = args.value(valueStr).toDouble(&ok);
						if (ok)
						{
							varValue = value;
						}
						else
							break;
					}
					//檢查是否為整數的區域變量
					else if (args.contains(valueStr) && args.value(valueStr).type() == QVariant::Int)
					{
						bool ok;
						int value = args.value(valueStr).toInt(&ok);
						if (ok)
						{
							varValue = value;
						}
						else
							break;
					}
				}

				QString key = QString("{:%1}").arg(i - kFormatPlaceHoldSize + 1);
				QString keyWithTime = QString("{T:%1}").arg(i - kFormatPlaceHoldSize + 1);
				if (formatStr.contains(key))
					formatStr.replace(key, varValue.toString());
				else if (formatStr.contains(keyWithTime))
				{
					formatStr.replace(key, formatTime(varValue.toInt()));
				}
			}
			else if (type == TK_REF)
			{
				QString subVarName = varValue.toString();
				if (subVarName.isEmpty() || !subVarName.startsWith(kVariablePrefix))
					continue;

				subVarName = subVarName.mid(1);

				if (!variables_->contains(subVarName))
					continue;

				QVariant subVarValue = variables_->value(subVarName);

				QString key = QString("{:%1}").arg(i - kFormatPlaceHoldSize + 1);
				QString keyWithTime = QString("{T:%1}").arg(i - kFormatPlaceHoldSize + 1);
				if (formatStr.contains(key))
					formatStr.replace(key, subVarValue.toString());
				else if (formatStr.contains(keyWithTime))
				{
					formatStr.replace(key, formatTime(subVarValue.toInt()));
				}
			}
		}

		if (varName.toLower() == "out")
		{
			const QDateTime time(QDateTime::currentDateTime());
			const QString timeStr(time.toString("hh:mm:ss:zzz"));
			QString src = "\0";

			QString msg = (QString("[%1 | @%2]: %3\0") \
				.arg(timeStr)
				.arg(lineNumber_ + 1, 3, 10, QLatin1Char(' ')).arg(formatStr));

			int color = QRandomGenerator::global()->bounded(0, 10);

			Injector& injector = Injector::getInstance();
			if (!injector.server.isNull())
				injector.server->announce(formatStr, color);
			if (!injector.scriptLogModel.isNull())
				injector.scriptLogModel->append(msg, color);
		}
		else if (varName.toLower() == "say")
		{
			int color = QRandomGenerator::global()->bounded(0, 10);
			Injector& injector = Injector::getInstance();
			if (!injector.server.isNull())
				injector.server->talk(formatStr, color);
		}
		else
			variables_->insert(varName, QVariant::fromValue(formatStr));
	} while (false);
}

//檢查"調用"是否傳入參數
void Parser::checkArgs()
{
	//check rest of the tokens is exist push to stack 	QStack<QVariantList> callArgs_
	QHash<QString, QVariant> args = getLabelVars();
	QVariantList list;
	for (int i = kCallPlaceHoldSize; i < tokens_.value(lineNumber_).size(); ++i)
	{
		Token token = tokens_.value(lineNumber_).value(i);
		QVariant var = token.data;
		if (token.type == TK_REF)
		{
			QString varName = token.data.toString().mid(1);
			if (!variables_->contains(varName))
			{
				list.clear();
				break;
			}

			var = variables_->value(varName);
			if (var.type() == QVariant::String)
			{
				token.type = TK_STRING;
				token.data = var.toString();

			}
			else if (var.type() == QVariant::Int)
			{
				token.type = TK_INT;
				token.data = var.toInt();
			}
			else if (var.type() == QVariant::Double)
			{
				token.type = TK_DOUBLE;
				token.data = var.toDouble();
			}
			else
			{
				list.clear();
				break;
			}

			var = token.data;
		}
		else if (token.type == TK_LABELVAR)
		{
			QString valueStr = token.data.toString();
			//檢查是否為字符串的區域變量
			if (args.contains(valueStr) && args.value(valueStr).type() == QVariant::String)
			{
				token.data = args.value(valueStr).toString();
			}
			//檢查是否為浮點數的區域變量
			else if (args.contains(valueStr) && args.value(valueStr).type() == QVariant::Double)
			{
				bool ok;
				double value = args.value(valueStr).toDouble(&ok);
				if (ok)
				{
					token.data = value;
				}
			}
			//檢查是否為整數的區域變量
			else if (args.contains(valueStr) && args.value(valueStr).type() == QVariant::Int)
			{
				bool ok;
				int value = args.value(valueStr).toInt(&ok);
				if (ok)
				{
					token.data = value;
				}
			}
		}

		if (token.type != TK_FUZZY)
			list.append(token.data);
		else
			list.append(0);
	}

	callArgsStack_.push(list);
}

//處理"調用"
bool Parser::processCall()
{
	RESERVE type = getTokenType(1);
	do
	{
		if (type != TK_NAME && type != TK_REF)
			break;

		if (type == TK_REF)
		{
			QString varName = getToken<QString>(1);
			if (varName.isEmpty() || !varName.startsWith(kVariablePrefix) || varName == QString(kVariablePrefix))
				break;

			varName = varName.mid(1);

			if (!variables_->contains(varName))
				break;

			QVariant var = variables_->value(varName);
			if (!var.isValid())
				break;

			bool ok;
			int jumpLineCount = var.toInt(&ok);
			if (ok)
			{
				callStack_.push(lineNumber_ + 1);
				checkArgs();
				jump(jumpLineCount, true);
				return true;
			}
			else
			{
				QString functionName = var.toString();
				if (functionName.isEmpty())
					break;

				int currentLine = lineNumber_;
				checkArgs();
				if (!jump(functionName, true))
				{
					break;
				}

				callStack_.push(currentLine + 1);
				return true;
			}
		}
		else
		{
			QString functionName = getToken<QString>(1);
			if (functionName.isEmpty())
				break;

			int jumpLine = matchLineFromLabel(functionName);
			if (jumpLine == -1)
				break;

			int currentLine = lineNumber_;
			checkArgs();
			if (!jump(functionName, true))
			{
				break;
			}
			callStack_.push(currentLine + 1); // Push the next line index to the call stack

			return true;
		}
	} while (false);
	return false;
}

//處理"跳轉"
bool Parser::processGoto()
{
	RESERVE type = getTokenType(1);
	do
	{
		if (type != TK_NAME && type != TK_INT && type != TK_REF)
			break;

		if (type == TK_NAME)
		{
			QString labelName = getToken<QString>(1);
			if (labelName.isEmpty())
				break;

			jump(labelName, false);
		}
		else if (type == TK_REF)
		{
			QString varName = getToken<QString>(1);
			if (varName.isEmpty() || !varName.startsWith(kVariablePrefix) || varName == QString(kVariablePrefix))
				break;

			varName = varName.mid(1);

			if (!variables_->contains(varName))
				break;

			QVariant var = variables_->value(varName);
			if (!var.isValid())
				break;

			bool ok;
			int jumpLineCount = var.toInt(&ok);
			if (ok)
			{
				jump(jumpLineCount, false);
				return true;
			}
			else
			{
				QString labelName = var.toString();
				if (labelName.isEmpty())
					break;

				if (!jump(labelName, false))
				{
					break;
				}

				return true;
			}
		}
		else
		{
			int jumpLineCount = getToken<int>(1);
			if (jumpLineCount == 0)
				break;

			jump(jumpLineCount, false);
			return true;
		}

	} while (false);
	return false;
}

//處理"指定行跳轉"
bool Parser::processJump()
{
	RESERVE type = getTokenType(1);
	do
	{
		if (type != TK_INT && type != TK_REF)
			break;

		if (type == TK_REF)
		{
			QString varName = getToken<QString>(1);
			if (varName.isEmpty() || !varName.startsWith(kVariablePrefix) || varName == QString(kVariablePrefix))
				break;

			varName = varName.mid(1);

			if (!variables_->contains(varName))
				break;

			QVariant var = variables_->value(varName);
			if (!var.isValid())
				break;

			bool ok;
			int line = var.toInt(&ok);
			if (ok && line > 0)
			{
				jumpto(line, false);
				return true;
			}
		}
		else
		{
			int line = getToken<int>(1);
			if (line <= 0)
				break;

			jumpto(line, false);
			return true;
		}
	} while (false);

	return false;
}

//處理"返回"
void Parser::processReturn()
{
	if (!callArgsStack_.isEmpty())
		callArgsStack_.pop();//call行號出棧
	if (!labalVarStack_.isEmpty())
		labalVarStack_.pop();//label局變量出棧
	if (!callStack_.isEmpty())
	{
		int returnIndex = callStack_.pop();
		int jumpLineCount = returnIndex - lineNumber_;
		jump(jumpLineCount, true);
		return;
	}
	lineNumber_ = 0;
}

//處理"返回跳轉"
void Parser::processBack()
{
	if (!jmpStack_.isEmpty())
	{
		int returnIndex = jmpStack_.pop();
		int jumpLineCount = returnIndex - lineNumber_;
		jump(jumpLineCount, true);
		return;
	}
	lineNumber_ = 0;
}

//處理"變量"運算
void Parser::variableCalculate(const QString& varName, RESERVE op, QVariant* pvar, const QVariant& varValue)
{
	if (nullptr == pvar)
		return;

	QVariant::Type type = pvar->type();

	switch (op)
	{
	case TK_ADD:
		if (type == QVariant::Double)
			*pvar = calc<double>(*pvar, varValue, op);
		else
			*pvar = calc<int>(*pvar, varValue, op);
		break;
	case TK_SUB:
		if (type == QVariant::Double)
			*pvar = calc<double>(*pvar, varValue, op);
		else
			*pvar = calc<int>(*pvar, varValue, op);
		break;
	case TK_MUL:
		if (type == QVariant::Double)
			*pvar = calc<double>(*pvar, varValue, op);
		else
			*pvar = calc<int>(*pvar, varValue, op);
		break;
	case TK_DIV:
		if (type == QVariant::Double)
			*pvar = calc<double>(*pvar, varValue, op);
		else
			*pvar = calc<int>(*pvar, varValue, op);
		break;
	case TK_INC:
		if (type == QVariant::Double)
			*pvar = calc<double>(*pvar, varValue, op);
		else
			*pvar = calc<int>(*pvar, varValue, op);
		break;
	case TK_DEC:
		if (type == QVariant::Double)
			*pvar = calc<double>(*pvar, varValue, op);
		else
			*pvar = calc<int>(*pvar, varValue, op);
		break;
	case TK_MOD:
		*pvar = pvar->toInt() % varValue.toInt();
		break;
	case TK_AND:
		*pvar = pvar->toInt() & varValue.toInt();
		break;
	case TK_OR:
		*pvar = pvar->toInt() | varValue.toInt();
		break;
	case TK_XOR:
		*pvar = pvar->toInt() ^ varValue.toInt();
		break;
	case TK_SHL:
		*pvar = pvar->toInt() << varValue.toInt();
		break;
	case TK_SHR:
		*pvar = pvar->toInt() >> varValue.toInt();
		break;
	default:
		Q_UNREACHABLE();//基於lexer事前處理過不可能會執行到這裡
		break;
	}
}

//處理錯誤
void Parser::handleError(int err)
{
	if (err == kNoChange)
		return;

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	if (err == kError)
		emit signalDispatcher.addErrorMarker(lineNumber_, err);

	QString msg;
	switch (err)
	{
	case kError:
		msg = QObject::tr("unknown error");
		break;
	case kArgError:
		msg = QObject::tr("argument error");
		break;
	case kLabelError:
		msg = QObject::tr("label incorrect or not exist");
		break;
	case kNoChange:
		return;
	default:
		break;
	}

	Injector& injector = Injector::getInstance();
	if (!injector.scriptLogModel.isNull())
		injector.scriptLogModel->append(QObject::tr("error occured at line %1. detail:%2").arg(lineNumber_ + 1).arg(msg), 6);
}

//解析跳轉棧或調用棧相關訊息並發送信號
void Parser::generateStackInfo(int type)
{
	if (mode_ != kSync)
		return;

	const QStack<int> stack = (type == 0) ? callStack_ : jmpStack_;
	QList<int> list = stack.toList();
	QHash <int, QString> hash;
	if (!list.isEmpty())
	{
		for (const int it : list)
		{
			if (!tokens_.contains(it - 1))
				continue;

			TokenMap tokens = tokens_.value(it - 1);
			if (tokens.isEmpty())
				continue;

			QStringList l;
			QString cmd = tokens.value(0).raw.trimmed() + " ";

			int size = tokens.size();
			for (int i = 1; i < size; ++i)
				l.append(tokens.value(i).raw.trimmed());

			hash.insert(it, cmd + l.join(", "));
		}
	}

	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance();
	if (type == 0)
		emit signalDispatcher.callStackInfoChanged(hash);
	else
		emit signalDispatcher.jumpStackInfoChanged(hash);
}