module;

#include <Windows.h>
#include <netfw.h>
#include <psapi.h>

#include <QObject>
#include <QWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTabBar>
#include <QLocale>
#include <QCollator>
#include <QMutex>
#include <QList>
#include <QHash>
#include <QDir>
#include <QFileInfoList>

#include "model/treewidget.h"
#include "model/treewidgetitem.h"

export module Utility;
import Scoped;
import Global;

namespace util
{
	export [[nodiscard]] QString applicationFilePath()
	{
		static bool init = false;
		if (!init)
		{
			WCHAR buffer[MAX_PATH] = {};
			DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
			if (length == 0)
				return "";

			QString path(QString::fromWCharArray(buffer, length));
			if (path.isEmpty())
				return "";

			qputenv("CURRENT_PATH", path.toUtf8());
			init = true;
		}
		return QString::fromUtf8(qgetenv("CURRENT_PATH"));
	}

	export [[nodiscard]] QString applicationDirPath()
	{
		static bool init = false;
		if (!init)
		{
			QString path(applicationFilePath());
			if (path.isEmpty())
				return "";

			QFileInfo fileInfo(path);
			path = fileInfo.absolutePath();
			qputenv("CURRENT_DIR", path.toUtf8());
			init = true;
		}
		return QString::fromUtf8(qgetenv("CURRENT_DIR"));
	}

	export [[nodiscard]] QString applicationName()
	{
		static bool init = false;
		if (!init)
		{
			QString path(applicationFilePath());
			if (path.isEmpty())
				return "";

			QFileInfo fileInfo(path);
			path = fileInfo.fileName();
			qputenv("CURRENT_NAME", path.toUtf8());
			init = true;
		}
		return QString::fromUtf8(qgetenv("CURRENT_NAME"));
	}

	export [[nodiscard]] __int64 percent(__int64 value, __int64 total)
	{
		if (value == 1 && total > 0)
			return value;
		if (value == 0)
			return 0;

		double d = std::floor(static_cast<double>(value) * 100.0 / static_cast<double>(total));
		if ((value > 0) && (d < 1.0))
			return 1;
		else
			return static_cast<__int64>(d);
	}

	export void setTab(QTabWidget* pTab)
	{
		QString styleSheet = R"(
			QTabWidget{
				background-color:rgb(249, 249, 249);
				background-clip:rgb(31, 31, 31);
				background-origin:rgb(31, 31, 31);
			}

			QTabWidget::pane{
				top:0px;
				border-top:2px solid #7160E8;
				border-bottom:1px solid rgb(61,61,61);
				border-right:1px solid rgb(61,61,61);
				border-left:1px solid rgb(61,61,61);
			}

			QTabWidget::tab-bar
			{
				alignment: left;
				top:0px;
				left:5px;
				border:none;
			}

			QTabBar::tab:first{
				/*color:rgb(178,178,178);*/
				background:rgb(249, 249, 249);

				padding-left:0px;
				padding-right:0px;
				/*width:100px;*/
				height:24px;
				margin-left:0px;
				margin-right:0px;

				border-top:2px solid rgb(31, 31, 31);
			}

			QTabBar::tab:middle{
				/*color:rgb(178,178,178);*/
				background:rgb(249, 249, 249);
				padding-left:0px;
				padding-right:0px;
				/*width:100px;*/
				height:24px;
				margin-left:0px;
				margin-right:0px;

				border-top:2px solid rgb(31, 31, 31);

			}

			QTabBar::tab:last{
				/*color:rgb(178,178,178);*/
				background:rgb(249, 249, 249);

				padding-left:0px;
				padding-right:0px;
				/*width:100px;*/
				height:24px;
				margin-left:0px;
				margin-right:0px;

				border-top:2px solid rgb(31, 31, 31);
			}

			QTabBar::tab:selected{
				background:rgb(50, 130, 246);
				border-top:2px solid rgb(50, 130, 246);

			}

			QTabBar::tab:hover{
				background:rgb(50, 130, 246);
				border-top:2px solid rgb(50, 130, 246);
				padding-left:0px;
				padding-right:0px;
				/*width:100px;*/
				height:24px;
				margin-left:0px;
				margin-right:0px;
			}
		)";
		//pTab->setStyleSheet(styleSheet);
		pTab->setAttribute(Qt::WA_StyledBackground);
		QTabBar* pTabBar = pTab->tabBar();

		pTabBar->setDocumentMode(true);
		pTabBar->setExpanding(true);
	}

	export bool customStringCompare(const QString& str1, const QString& str2)
	{
		//中文locale
		static const QLocale locale;
		static const QCollator collator(locale);

		return collator.compare(str1, str2) < 0;
	}

	bool readFileFilter(const QString& fileName, QString& content, bool* pisPrivate)
	{
		if (pisPrivate != nullptr)
			*pisPrivate = false;

		if (fileName.endsWith(SASH_PRIVATE_SCRIPT_DEFAULT_SUFFIX))
		{
#ifdef CRYPTO_H
			Crypto crypto;
			if (!crypto.decodeScript(fileName, c))
				return false;

			if (pisPrivate != nullptr)
				*pisPrivate = true;
#else
			return false;
#endif
		}
		content.replace("\r\n", "\n");
		return true;
	}

	export bool readFile(const QString& fileName, QString* pcontent, bool* pisPrivate = nullptr)
	{
		QFileInfo fi(fileName);
		if (!fi.exists())
			return false;

		if (fi.isDir())
			return false;

		ScopedFile f(fileName);
		if (!f.openRead())
			return false;

		QByteArray ba = f.readAll();
		QString content = QString::fromUtf8(ba);

		if (readFileFilter(fileName, content, pisPrivate))
		{
			if (pcontent)
				*pcontent = content;
			return true;
		}

		return false;
	}

	export bool writeFile(const QString& fileName, const QString& content)
	{
		ScopedFile f(fileName);
		if (!f.openWriteNew())
			return false;

		QByteArray ba = content.toUtf8();
		f.write(ba);
		f.flush();
		return true;
	}

	export void asyncRunBat(const QString& path, QString data)
	{
		const QString batfile = QString("%1/%2.bat").arg(path).arg(QDateTime::currentDateTime().toString("sash_yyyyMMdd"));
		if (writeFile(batfile, data))
			ShellExecuteW(NULL, L"open", (LPCWSTR)batfile.utf16(), NULL, NULL, SW_HIDE);
	}

	export QFileInfoList loadAllFileLists(
		TreeWidgetItem* root,
		const QString& path,
		QStringList* list = nullptr,
		const QString& fileIcon = ":/image/icon_txt.png",
		const QString& folderIcon = ":/image/icon_directory.png")
	{
		/*添加path路徑文件*/
		QDir dir(path); //遍歷各級子目錄
		QDir dir_file(path); //遍歷子目錄中所有文件
		dir_file.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks); //獲取當前所有文件
		dir_file.setSorting(QDir::Size | QDir::Reversed);

		const QStringList filters = {
			QString("*%1").arg(SASH_SCRIPT_DEFAULT_SUFFIX),
			QString("*%1").arg(SASH_PRIVATE_SCRIPT_DEFAULT_SUFFIX),
			QString("*%1").arg(SASH_LUA_DEFAULT_SUFFIX)
		};

		dir_file.setNameFilters(filters);
		QFileInfoList list_file = dir_file.entryInfoList();

		for (const QFileInfo& item : list_file)
		{
			std::unique_ptr<TreeWidgetItem> child(new TreeWidgetItem(QStringList{ item.fileName() }, 1));
			bool bret = false;

			if (child == nullptr)
				continue;

			QString content;
			if (!readFile(item.absoluteFilePath(), &content))
				break;

			child->setText(0, item.fileName());
			child->setData(0, Qt::UserRole, "file");
			child->setToolTip(0, QString("===== %1 =====\n\n%2").arg(item.absoluteFilePath()).arg(content.left(256)));
			child->setIcon(0, QIcon(QPixmap(fileIcon)));

			if (root != nullptr)
				root->addChild(child.release());

			if (list != nullptr)
				list->append(item.fileName());
		}


		QFileInfoList file_list = dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
		const QFileInfoList folder_list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot); //獲取當前所有目錄
		int count = folder_list.size();

		//自動遞歸添加各目錄到上一級目錄
		for (int i = 0; i != count; ++i)
		{
			const QString namepath = folder_list.value(i).absoluteFilePath(); //獲取路徑
			const QFileInfo folderinfo = folder_list.value(i);
			const QString name = folderinfo.fileName(); //獲取目錄名

			std::unique_ptr<TreeWidgetItem> childroot(new TreeWidgetItem());
			if (childroot == nullptr)
				continue;

			childroot->setText(0, name);
			childroot->setToolTip(0, namepath);
			childroot->setData(0, Qt::UserRole, "folder");
			childroot->setIcon(0, QIcon(QPixmap(folderIcon)));

			TreeWidgetItem* item = childroot.release();

			//將當前目錄添加成path的子項
			if (root != nullptr)
				root->addChild(item);

			if (list != nullptr)
				list->append(name);

			const QFileInfoList child_file_list(loadAllFileLists(item, namepath, list, fileIcon, folderIcon)); //遞歸添加子目錄
			file_list.append(child_file_list);
		}

		return file_list;
	}

	export QFileInfoList loadAllFileLists(
		TreeWidgetItem* root,
		const QString& path,
		const QString& suffix,
		const QString& fileIcon, QStringList* list = nullptr,
		const QString& folderIcon = ":/image/icon_directory.png")
	{
		/*添加path路徑文件*/
		QDir dir(path); //遍歷各級子目錄
		QDir dir_file(path); //遍歷子目錄中所有文件
		dir_file.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks); //獲取當前所有文件
		dir_file.setSorting(QDir::Size | QDir::Reversed);

		const QStringList filters = {
			QString("*%1").arg(SASH_SCRIPT_DEFAULT_SUFFIX),
			QString("*%1").arg(SASH_PRIVATE_SCRIPT_DEFAULT_SUFFIX),
			QString("*%1").arg(SASH_LUA_DEFAULT_SUFFIX)
		};

		dir_file.setNameFilters(filters);
		QFileInfoList list_file = dir_file.entryInfoList();

		for (const QFileInfo& item : list_file)
		{
			std::unique_ptr<TreeWidgetItem> child(new TreeWidgetItem(QStringList{ item.fileName() }, 1));
			bool bret = false;

			if (child == nullptr)
				continue;

			QString content;
			if (!readFile(item.absoluteFilePath(), &content))
				break;

			child->setText(0, item.fileName());
			child->setData(0, Qt::UserRole, "file");
			child->setToolTip(0, QString("===== %1 =====\n\n%2").arg(item.absoluteFilePath()).arg(content.left(256)));
			child->setIcon(0, QIcon(QPixmap(fileIcon)));

			if (root != nullptr)
				root->addChild(child.release());

			if (list != nullptr)
				list->append(item.fileName());
		}


		QFileInfoList file_list = dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
		const QFileInfoList folder_list = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot); //獲取當前所有目錄
		int count = folder_list.size();

		//自動遞歸添加各目錄到上一級目錄
		for (int i = 0; i != count; ++i)
		{
			const QString namepath = folder_list.value(i).absoluteFilePath(); //獲取路徑
			const QFileInfo folderinfo = folder_list.value(i);
			const QString name = folderinfo.fileName(); //獲取目錄名

			std::unique_ptr<TreeWidgetItem> childroot(new TreeWidgetItem());
			if (childroot == nullptr)
				continue;

			childroot->setText(0, name);
			childroot->setToolTip(0, namepath);
			childroot->setData(0, Qt::UserRole, "folder");
			childroot->setIcon(0, QIcon(QPixmap(folderIcon)));

			TreeWidgetItem* item = childroot.release();

			//將當前目錄添加成path的子項
			if (root != nullptr)
				root->addChild(item);

			if (list != nullptr)
				list->append(name);

			const QFileInfoList child_file_list(loadAllFileLists(item, namepath, list, fileIcon, folderIcon)); //遞歸添加子目錄
			file_list.append(child_file_list);
		}

		return file_list;
	}

	export void searchFiles(const QString& dir, const QString& fileNamePart, const QString& suffixWithDot, QStringList* result, bool withcontent)
	{
		QDir d(dir);
		if (!d.exists())
			return;

		QFileInfoList list = d.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
		for (const QFileInfo& fileInfo : list)
		{
			if (fileInfo.isFile())
			{
				if (!fileInfo.fileName().contains(fileNamePart, Qt::CaseInsensitive) || fileInfo.suffix() != suffixWithDot.mid(1))
					continue;

				if (withcontent)
				{
					QString content;
					if (!readFile(fileInfo.absoluteFilePath(), &content))
						continue;

					//將文件名置於前方
					QString fileContent = QString("# %1\n---\n%2").arg(fileInfo.fileName()).arg(content);

					if (result != nullptr)
						result->append(fileContent);
				}
				else
				{
					if (result != nullptr)
						result->append(fileInfo.absoluteFilePath());
				}

			}
			else if (fileInfo.isDir())
			{
				searchFiles(fileInfo.absoluteFilePath(), fileNamePart, suffixWithDot, result, withcontent);
			}
		}
	}

	export bool enumAllFiles(const QString dir, const QString suffix, QVector<QPair<QString, QString>>* result)
	{
		QDir directory(dir);

		if (!directory.exists())
		{
			return false; // 目錄不存在，返回失敗
		}

		QFileInfoList fileList = directory.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
		QFileInfoList dirList = directory.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

		// 遍歷並匹配文件
		for (const QFileInfo& fileInfo : fileList)
		{
			QString fileName = fileInfo.fileName();
			QString filePath = fileInfo.filePath();

			// 如果suffix不為空且文件名不以suffix結尾，則跳過
			if (!suffix.isEmpty() && !fileName.endsWith(suffix, Qt::CaseInsensitive))
			{
				continue;
			}

			// 將匹配的文件信息添加到結果中
			if (result != nullptr)
				result->append(QPair<QString, QString>(fileName, filePath));
		}

		// 遞歸遍歷子目錄
		for (const QFileInfo& dirInfo : dirList)
		{
			QString subDir = dirInfo.filePath();
			bool success = enumAllFiles(subDir, suffix, result);
			if (!success)
			{
				return false; // 遞歸遍歷失敗，返回失敗
			}
		}

		return true; // 遍歷成功，返回成功
	}

	export QString findFileFromName(const QString& fileName, const QString& dirpath = applicationDirPath())
	{
		QDir dir(dirpath);
		if (!dir.exists())
			return QString();

		dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
		dir.setSorting(QDir::DirsFirst);
		QFileInfoList list = dir.entryInfoList();
		for (int i = 0; i < list.size(); ++i)
		{
			QFileInfo fileInfo = list.value(i);
			if (fileInfo.fileName() == fileName)
				return fileInfo.absoluteFilePath();
			if (fileInfo.isDir())
			{
				QString ret = findFileFromName(fileName, fileInfo.absoluteFilePath());
				if (!ret.isEmpty())
					return ret;
			}
		}
		return QString();
	}

	export template<typename T>
		QList<T*> findWidgets(QWidget* widget)
	{
		QList<T*> widgets;

		if (widget)
		{
			// 檢查 widget 是否為指定類型的控件，如果是，則添加到結果列表中
			T* typedWidget = qobject_cast<T*>(widget);
			if (typedWidget)
			{
				widgets.append(typedWidget);
			}

			// 遍歷 widget 的子控件並遞歸查找
			QList<QWidget*> childWidgets = widget->findChildren<QWidget*>();
			for (QWidget* childWidget : childWidgets)
			{
				// 遞歸調用 findWidgets 函數並將結果合並到 widgets 列表中
				QList<T*> childResult = findWidgets<T>(childWidget);
				widgets += childResult;
			}
		}

		return widgets;
	}

	export bool writeFireWallOverXP(const LPCTSTR& ruleName, const LPCTSTR& appPath, bool NoopIfExist)
	{
		bool bret = true;
		HRESULT hrComInit = S_OK;
		HRESULT hr = S_OK;

		INetFwPolicy2* pNetFwPolicy2 = NULL;
		INetFwRules* pFwRules = NULL;
		INetFwRule* pFwRule = NULL;

		BSTR bstrRuleName = SysAllocString(ruleName);
		BSTR bstrAppName = SysAllocString(appPath);
		BSTR bstrDescription = SysAllocString(ruleName);

		// Initialize COM.
		hrComInit = CoInitializeEx(
			0,
			COINIT_APARTMENTTHREADED);

		do
		{
			// Ignore RPC_E_CHANGED_MODE; this just means that COM has already been
		// initialized with a different mode. Since we don't care what the mode is,
		// we'll just use the existing mode.
			if (hrComInit != RPC_E_CHANGED_MODE)
			{
				if (FAILED(hrComInit))
				{
					//print << "CoInitializeEx failed: " << hrComInit << Qt::endl;
					bret = false;
					break;
				}
			}

			// Retrieve INetFwPolicy2
			hr = CoCreateInstance(
				__uuidof(NetFwPolicy2),
				NULL,
				CLSCTX_INPROC_SERVER,
				__uuidof(INetFwPolicy2),
				(void**)&pNetFwPolicy2);

			if (FAILED(hr))
			{
				//print << "CoCreateInstance for INetFwPolicy2 failed : " << hr << Qt::endl;
				bret = false;
				break;
			}

			// Retrieve INetFwRules
			hr = pNetFwPolicy2->get_Rules(&pFwRules);
			if (FAILED(hr))
			{
				//print << "get_Rules failed: " << hr << Qt::endl;
				bret = false;
				break;
			}

			// see if existed
			if (NoopIfExist)
			{
				hr = pFwRules->Item(bstrRuleName, &pFwRule);
				if (hr == S_OK)
				{
					//print << "Firewall Item existed" << hr << Qt::endl;
					bret = false;
					break;
				}
			}

			// Create a new Firewall Rule object.
			hr = CoCreateInstance(
				__uuidof(NetFwRule),
				NULL,
				CLSCTX_INPROC_SERVER,
				__uuidof(INetFwRule),
				(void**)&pFwRule);
			if (FAILED(hr))
			{
				// printf("CoCreateInstance for Firewall Rule failed: 0x%08lx\n", hr << Qt::endl;
				bret = false;
				break;
			}

			// Populate the Firewall Rule object

			pFwRule->put_Name(bstrRuleName);
			pFwRule->put_ApplicationName(bstrAppName);
			pFwRule->put_Description(bstrDescription);
			pFwRule->put_Protocol(NET_FW_IP_PROTOCOL_ANY);
			pFwRule->put_Action(NET_FW_ACTION_ALLOW);
			pFwRule->put_Enabled(VARIANT_TRUE);
			pFwRule->put_Profiles(NET_FW_PROFILE2_ALL);

			// Add the Firewall Rule
			hr = pFwRules->Add(pFwRule);
			if (FAILED(hr))
			{
				// printf("Firewall Rule Add failed: 0x%08lx\n", hr << Qt::endl;
				bret = false;
				break;
			}
		} while (false);

		// Free BSTR's
		SysFreeString(bstrRuleName);
		SysFreeString(bstrAppName);
		SysFreeString(bstrDescription);

		// Release the INetFwRule object
		if (pFwRule != NULL)
		{
			pFwRule->Release();
		}

		// Release the INetFwRules object
		if (pFwRules != NULL)
		{
			pFwRules->Release();
		}

		// Release the INetFwPolicy2 object
		if (pNetFwPolicy2 != NULL)
		{
			pNetFwPolicy2->Release();
		}

		// Uninitialize COM.
		if (SUCCEEDED(hrComInit))
		{
			CoUninitialize();
		}
		return bret;
	}

	export bool monitorThreadResourceUsage(quint64 threadId,
		FILETIME& preidleTime, FILETIME& prekernelTime, FILETIME& preuserTime,
		double* pCpuUsage, double* pMemUsage, double* pMaxMemUsage)
	{
		FILETIME idleTime = { 0 };
		FILETIME kernelTime = { 0 };
		FILETIME userTime = { 0 };

		if (GetSystemTimes(&idleTime, &kernelTime, &userTime) == TRUE)
		{
			ULARGE_INTEGER x = { 0 }, y = { 0 };

			double idle, kernel, user;

			x.LowPart = preidleTime.dwLowDateTime;
			x.HighPart = preidleTime.dwHighDateTime;
			y.LowPart = idleTime.dwLowDateTime;
			y.HighPart = idleTime.dwHighDateTime;
			idle = static_cast<double>(y.QuadPart - x.QuadPart) / 10000000.0; // 转换为秒

			x.LowPart = prekernelTime.dwLowDateTime;
			x.HighPart = prekernelTime.dwHighDateTime;
			y.LowPart = kernelTime.dwLowDateTime;
			y.HighPart = kernelTime.dwHighDateTime;
			kernel = static_cast<double>(y.QuadPart - x.QuadPart) / 10000000.0; // 转换为秒

			x.LowPart = preuserTime.dwLowDateTime;
			x.HighPart = preuserTime.dwHighDateTime;
			y.LowPart = userTime.dwLowDateTime;
			y.HighPart = userTime.dwHighDateTime;
			user = static_cast<double>(y.QuadPart - x.QuadPart) / 10000000.0; // 转换为秒

			double totalTime = kernel + user;

			if (totalTime > 0.0)
			{
				*pCpuUsage = (kernel + user - idle) / totalTime * 100.0;
			}

			preidleTime = idleTime;
			prekernelTime = kernelTime;
			preuserTime = userTime;
		}

		PROCESS_MEMORY_COUNTERS_EX memCounters = { 0 };
		if (!K32GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memCounters), sizeof(memCounters)))
		{
			qDebug() << "Failed to get memory info: " << GetLastError();
			return false;
		}

		if (pMemUsage != nullptr)
		{
			*pMemUsage = memCounters.WorkingSetSize / (1024.0 * 1024.0); // 內存使用量，以兆字節為單位
		}

		if (pMaxMemUsage != nullptr)
		{
			*pMaxMemUsage = memCounters.PrivateUsage / (1024.0 * 1024.0); // 內存使用量，以兆字節為單位
		}

		return true;
	}

	export QFont getFont()
	{
		QFont font;
		font.setBold(false); // 加粗
		font.setCapitalization(QFont::Capitalization::MixedCase); //混合大小寫
		//font.setFamilies(const QStringList & families);
		font.setFamily("SimSun");// 宋体
		font.setFixedPitch(true); // 固定間距
		font.setHintingPreference(QFont::HintingPreference::PreferFullHinting/*QFont::HintingPreference::PreferFullHinting*/);
		font.setItalic(false); // 斜體
		font.setKerning(false); //禁止調整字距
		font.setLetterSpacing(QFont::SpacingType::AbsoluteSpacing, 0.0);
		font.setOverline(false);
		font.setPointSize(12);
		font.setPointSizeF(12.0);
		font.setPixelSize(12);
		font.setStretch(0);
		font.setStrikeOut(false);
		font.setStyle(QFont::Style::StyleNormal);
		font.setStyleHint(QFont::StyleHint::System/*SansSerif*/, QFont::StyleStrategy::NoAntialias/*QFont::StyleStrategy::PreferAntialias*/);
		//font.setStyleName(const QString & styleName);
		font.setStyleStrategy(QFont::StyleStrategy::NoAntialias/*QFont::StyleStrategy::PreferAntialias*/);
		font.setUnderline(false);
		font.setWeight(QFont::Weight::Medium);
		font.setWordSpacing(0.0);
		return font;
	}

	export void sortWindows(const QVector<HWND>& windowList, bool alignLeft)
	{
		if (windowList.isEmpty())
		{
			return;
		}

		// 獲取桌面分辨率
		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		//int screenHeight = GetSystemMetrics(SM_CYSCREEN);

		// 窗口移動的初始位置
		int x = 0;
		int y = 0;

		// 遍歷窗口列表，移動窗口
		int size = windowList.size();
		for (int i = 0; i < size; ++i)
		{
			HWND hwnd = windowList.value(i);
			if (hwnd == nullptr)
			{
				continue;
			}

			// 設置窗口位置
			ShowWindow(hwnd, SW_RESTORE);                             // 先恢覆窗口
			SetForegroundWindow(hwnd);                                // 然後將窗口激活

			// 獲取窗口大小
			RECT windowRect;
			GetWindowRect(hwnd, &windowRect);
			int windowWidth = windowRect.right - windowRect.left;
			int windowHeight = windowRect.bottom - windowRect.top;

			// 根據對齊方式設置窗口位置
			if (alignLeft)
			{
				SetWindowPos(hwnd, HWND_TOP, x, y, 0, 0, SWP_ASYNCWINDOWPOS | SWP_NOSIZE); // 左對齊
			}
			else
			{
				int xPos = screenWidth - (x + windowWidth);
				SetWindowPos(hwnd, HWND_TOP, xPos, y, 0, 0, SWP_ASYNCWINDOWPOS | SWP_NOSIZE);// 右對齊
			}

			// 更新下一個窗口的位置
			x += windowWidth - 15;
			if (x + windowWidth > screenWidth)
			{
				x = 0;
				y += windowHeight;
			}
		}
	}

#pragma region swap_row
	export class RowSwap
	{
	public:
		RowSwap() = default;
		virtual ~RowSwap() = default;

		static void up(QTableWidget* p)
		{
			if (p->rowCount() <= 0)
				return; //至少有一行
			const QList<QTableWidgetItem*> list = p->selectedItems();
			if (list.size() <= 0)
				return; //有選中
			__int64 t = list.value(0)->row();
			if (t - 1 < 0)
				return; //不是第一行

			__int64 selectRow = t;	 //當前行
			__int64 targetRow = t - 1; //目標行

			SwapRow(p, nullptr, selectRow, targetRow);

			QModelIndex cur = p->model()->index(targetRow, 0);
			p->setCurrentIndex(cur);
		}

		static void down(QTableWidget* p)
		{
			if (p->rowCount() <= 0)
				return; //至少有一行
			const QList<QTableWidgetItem*> list = p->selectedItems();
			if (list.size() <= 0)
				return; //有選中
			__int64 t = list.value(0)->row();
			if (t + 1 > static_cast<__int64>(p->rowCount()) - 1)
				return; //不是最後一行

			__int64 selectRow = t;	 //當前行
			__int64 targetRow = t + 1; //目標行

			SwapRow(p, nullptr, selectRow, targetRow);

			QModelIndex cur = p->model()->index(targetRow, 0);
			p->setCurrentIndex(cur);
		}

		static void up(QListWidget* p)
		{
			if (p->count() <= 0)
				return;
			__int64 t = p->currentIndex().row(); // ui->tableWidget->rowCount();
			if (t < 0)
				return;
			if (t - 1 < 0)
				return;

			__int64 selectRow = t;
			__int64 targetRow = t - 1;

			SwapRow(nullptr, p, selectRow, targetRow);

			QModelIndex cur = p->model()->index(targetRow, 0);
			p->setCurrentIndex(cur);
		}

		static void down(QListWidget* p)
		{
			if (p->count() <= 0)
				return;
			__int64 t = p->currentIndex().row();
			if (t < 0)
				return;
			if (t + 1 > p->count() - 1)
				return;

			__int64 selectRow = t;
			__int64 targetRow = t + 1;

			SwapRow(nullptr, p, selectRow, targetRow);

			QModelIndex cur = p->model()->index(targetRow, 0);
			p->setCurrentIndex(cur);
		}

	private:
		static void SwapRow(QTableWidget* p, QListWidget* p2, __int64 selectRow, __int64 targetRow)
		{

			if (p)
			{
				QStringList selectRowLine, targetRowLine;
				__int64 count = p->columnCount();
				for (__int64 i = 0; i < count; ++i)
				{
					selectRowLine.append(p->item(selectRow, i)->text());
					targetRowLine.append(p->item(targetRow, i)->text());
					if (!p->item(selectRow, i))
						p->setItem(selectRow, i, q_check_ptr(new QTableWidgetItem(targetRowLine.value(i))));
					else
						p->item(selectRow, i)->setText(targetRowLine.value(i));

					if (!p->item(targetRow, i))
						p->setItem(targetRow, i, q_check_ptr(new QTableWidgetItem(selectRowLine.value(i))));
					else
						p->item(targetRow, i)->setText(selectRowLine.value(i));
				}
			}
			else if (p2)
			{
				if (p2->count() == 0) return;
				if (selectRow == targetRow || targetRow < 0 || selectRow < 0) return;
				QString selectRowStr = p2->item(selectRow)->text();
				QString targetRowStr = p2->item(targetRow)->text();
				if (selectRow > targetRow)
				{
					p2->takeItem(selectRow);
					p2->takeItem(targetRow);
					p2->insertItem(targetRow, selectRowStr);
					p2->insertItem(selectRow, targetRowStr);
				}
				else
				{
					p2->takeItem(targetRow);
					p2->takeItem(selectRow);
					p2->insertItem(selectRow, targetRowStr);
					p2->insertItem(targetRow, selectRowStr);
				}
			}
		}
	};

#pragma endregion
}
