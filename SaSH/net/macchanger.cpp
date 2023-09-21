/*
* Copyright (C) 2016 SaEeD
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
*
*/
#include "stdafx.h"
#include "macchanger.h"
using namespace std;
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

#pragma comment(lib, "iphlpapi.lib")


#include <QObject>

MyMACAddr::MyMACAddr()
{
	//Seeding for random numbers
	srand((unsigned)time(0));
}

MyMACAddr::~MyMACAddr()
{
}

//-----------------------------------------------
// Generate Random MAC Addresses
//-----------------------------------------------
string MyMACAddr::GenRandMAC()
{
	stringstream temp;
	int number = 0;
	string result;

	for (int i = 0; i < 6; ++i)
	{
		number = rand() % 254;
		temp << setfill('0') << setw(2) << hex << number;
		if (i != 5)
		{
			temp << "-";
		}
	}
	result = temp.str();

	for (char& c : result)
	{
		c = toupper(c);
	}

	return result;
}

//-----------------------------------------------
// Original Code from : https://msdn.microsoft.com/en-us/library/windows/desktop/aa366062(v=vs.85).aspx
// Print the details of Network Interfaces  
//-----------------------------------------------

void MyMACAddr::showAdapterList()
{
	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;
	UINT i;

	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	pAdapterInfo = (IP_ADAPTER_INFO*)MALLOC(sizeof(IP_ADAPTER_INFO));
	if (pAdapterInfo == NULL)
	{
		wcerr << L"分配所需的記憶體錯誤 調用 \"獲取網路介面卡資訊。\"" << endl;
	}
	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		FREE(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO*)MALLOC(ulOutBufLen);
		if (pAdapterInfo == NULL)
		{
			wcerr << L"分配所需的記憶體錯誤 調用 \"獲取網路介面卡資訊。\"" << endl;
		}
	}

	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR)
	{
		pAdapter = pAdapterInfo;
		while (pAdapter)
		{
			std::wcout << L"\n\t網路介面卡索引： \t" << pAdapter->ComboIndex << endl;
			std::wcout << L"\t網路介面卡名稱： \t" << QString(pAdapter->AdapterName).toStdWString().c_str() << endl;
			std::wcout << L"\t網路介面卡描述： \t" << pAdapter->Description << endl;
			std::wcout << L"\t網路介面卡地址： \t";
			for (i = 0; i < pAdapter->AddressLength; ++i)
			{
				if (i == (pAdapter->AddressLength - 1))
					printf("%.2X\n", (int)pAdapter->Address[i]);
				else
					printf("%.2X-", (int)pAdapter->Address[i]);
			}
			std::wcout << L"\tIP 地址： \t" << QString(pAdapter->IpAddressList.IpAddress.String).toStdWString().c_str() << endl;
			std::wcout << L"\tIP 網路遮罩： \t" << QString(pAdapter->IpAddressList.IpMask.String).toStdWString().c_str() << endl;
			std::wcout << L"\t預設閘道： \t" << QString(pAdapter->GatewayList.IpAddress.String).toStdWString().c_str() << endl;
			pAdapter = pAdapter->Next;
		}
	}
	else
	{
		wcerr << L"獲取網路介面卡訊息失敗，錯誤： " << dwRetVal << endl;
	}
	if (pAdapterInfo)
		FREE(pAdapterInfo);
}

//-----------------------------------------------
// Get Network Adapter's Name and MAC addresses
//-----------------------------------------------
unordered_map<string, string> MyMACAddr::getAdapters()
{
	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;

	unordered_map<string, string> result;
	stringstream temp;
	string str_mac;

	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	pAdapterInfo = (IP_ADAPTER_INFO*)MALLOC(sizeof(IP_ADAPTER_INFO));
	if (pAdapterInfo == NULL)
	{
		//wcerr << L"分配所需的記憶體錯誤 調用 \"獲取網路介面卡資訊\"。" << endl;
	}
	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		FREE(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO*)MALLOC(ulOutBufLen);
		if (pAdapterInfo == NULL)
		{
			//wcerr << L"分配調用 \"獲取網路介面卡資訊\" 所需的記憶體時出錯。\n" << endl;
		}
	}

	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR)
	{
		pAdapter = pAdapterInfo;
		while (pAdapter)
		{
			for (UINT i = 0; i < pAdapter->AddressLength; ++i)
			{
				temp << setfill('0') << setw(2) << hex << (int)pAdapter->Address[i];
				if (i != pAdapter->AddressLength - 1)
				{
					temp << "-";
				}
			}
			str_mac = temp.str();
			temp.str("");
			std::ignore = temp.rdbuf();
			for (char& c : str_mac)
			{
				c = toupper(c);
			}

			result.insert({ pAdapter->Description, str_mac });
			pAdapter = pAdapter->Next;
		}
	}
	else
	{
		//wcerr << L"獲取網路介面卡訊息失敗，錯誤： " << dwRetVal << endl;
	}
	if (pAdapterInfo)
		FREE(pAdapterInfo);

	return result;
}

//-----------------------------------------------
// Assing Random MAC address to Network Interface
//-----------------------------------------------
std::string MyMACAddr::AssingRndMAC()
{
	std::string retstring = "\0";
	//-------- Copy Network interfaces to Vector
	vector <string> list;
	unordered_map<string, string> AdapterDetails = getAdapters();
	for (const std::pair<const std::string, std::string>& itm : AdapterDetails)
	{
		list.push_back(itm.first);
	}

	//wcout << L"\n[+]可用的網路介面卡列表： " << endl;
	int range = 0;
	for (std::vector<std::string>::iterator itm = list.begin(); itm != list.end(); itm++)
	{
		cout << '\t' << range + 1 << ")" << *itm << endl;
		range++;
	}

	//wcout << L"[*]選擇： ";
	//-------- Input validation
	int selection = 1;
	//cin >> selection;
	//if (cin.fail() || (selection < 1) || (selection > range))
	//{
	//	cin.clear();
	//	cin.ignore(numeric_limits<streamsize>::max(), '\n');
	//	//wcerr << L"[!]無效的選擇！" << endl;
	//	return "\0";
	//}

	//wcout << L"----------------------------------------------" << endl;
	//wcout << L"[-]所選網路介面卡是： " << QString(list.at(selection - 1).c_str()).toStdWString().c_str() << endl;
	//wcout << L"[+]舊的MAC是： " << QString(AdapterDetails.at(list.at(selection - 1)).c_str()).toStdWString().c_str() << endl;

	//-------- Converting to Wide characters
	wstring wstr(list.at(selection - 1).begin(), list.at(selection - 1).end());
	const wchar_t* wAdapterName = wstr.c_str();

	//-------- Registry Key for Network Interfaces
	HKEY hKey = NULL;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		_T("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002bE10318}"),
		0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{

		DWORD dwIndex = 0;
		TCHAR Name[1024];
		DWORD cName = 1024;
		while (RegEnumKeyEx(hKey, dwIndex, Name, &cName,
			NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			HKEY hSubKey = NULL;
			if (RegOpenKeyEx(hKey, Name, 0, KEY_ALL_ACCESS, &hSubKey) == ERROR_SUCCESS)
			{
				BYTE Data[1204];
				DWORD cbData = 1024;
				if (RegQueryValueEx(hSubKey, _T("DriverDesc"), NULL, NULL, Data, &cbData) == ERROR_SUCCESS)
				{

					if (_tcscmp((TCHAR*)Data, wAdapterName) == 0)
					{
						string temp = GenRandMAC();
						string newMAC = temp;
						temp.erase(std::remove(temp.begin(), temp.end(), '-'), temp.end());

						wstring wstr_newMAC(temp.begin(), temp.end());
						const wchar_t* newMACAddr = wstr_newMAC.c_str();

						//--------Add new MAC to Registry Subkey and disable and re-enable the interface
						// to effect changes
						if (RegSetValueEx(hSubKey, _T("NetworkAddress"), 0, REG_SZ,
							(const BYTE*)newMACAddr, sizeof(TCHAR) * ((DWORD)_tcslen(newMACAddr) + 1)) == ERROR_SUCCESS)
						{
							retstring = newMAC;
							//wcout << L"[+]新的隨機MAC: " << QString(newMAC.c_str()).toUtf8().constData().c_str() << endl;
							DisableEnableConnections(false, wAdapterName);
							DisableEnableConnections(true, wAdapterName);
						}
					}
				}
				RegCloseKey(hSubKey);
			}
			cName = 1024;
			dwIndex++;
		}
		RegCloseKey(hKey);
	}
	else
	{
		//wcerr << L"[!]無法訪問註冊表 - 也許您沒有管理員權限。" << endl;
		return "\0";
	}
	//wcout << L"----------------------------------------------" << endl;
	return retstring;
}

//-----------------------------------------------
// Original Code: https://social.msdn.microsoft.com/Forums/en-US/ad3ae21d-515d-4f67-8519-216f1058e390/enabledisable-network-card?forum=netfxnetcom
// Modified to only Disable and Re-enable the given Adapter 
//-----------------------------------------------
HRESULT MyMACAddr::DisableEnableConnections(BOOL bEnable, const wchar_t* AdapterName)
{
	HRESULT hr = E_FAIL;

	Q_UNUSED(CoInitialize(NULL));

	INetConnectionManager* pNetConnectionManager = NULL;
	hr = CoCreateInstance(CLSID_ConnectionManager,
		NULL,
		CLSCTX_LOCAL_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
		IID_INetConnectionManager,
		reinterpret_cast<LPVOID*>(&pNetConnectionManager)
	);
	if (SUCCEEDED(hr))
	{
		/*
		Get an enumurator for the set of connections on the system
		*/
		IEnumNetConnection* pEnumNetConnection;
		pNetConnectionManager->EnumConnections(NCME_DEFAULT, &pEnumNetConnection);

		ULONG ulCount = 0;
		BOOL fFound = FALSE;
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

		HRESULT hrT = S_OK;

		/*
		Enumurate through the list of adapters on the system and look for the one we want
		NOTE: To include per-user RAS connections in the list, you need to set the COM
		Proxy Blanket on all the interfaces. This is not needed for All-user RAS
		connections or LAN connections.
		*/
		do
		{
			NETCON_PROPERTIES* pProps = NULL;
			INetConnection* pConn;

			/*
			Find the next (or first connection)
			*/
			hrT = pEnumNetConnection->Next(1, &pConn, &ulCount);

			if (SUCCEEDED(hrT) && 1 == ulCount)
			{
				/*
				Get the connection properties
				*/
				hrT = pConn->GetProperties(&pProps);

				if (S_OK == hrT)
				{
					//printf("* %S\n", pProps->pszwName);

					if (bEnable && (_tcscmp((TCHAR*)pProps->pszwDeviceName, AdapterName) == 0))
					{
						//printf("      Enabling adapter: %S...\n", pProps->pszwName);
						//wprintf(L"[+]啟用中的網路介面卡: %s...\n", pProps->pszwDeviceName);
						hr = pConn->Connect();
					}
					else if (_tcscmp((TCHAR*)pProps->pszwDeviceName, AdapterName) == 0)
					{
						//wprintf(L"[+]禁用中的網路介面卡: %s...\n", pProps->pszwDeviceName);
						hr = pConn->Disconnect();
					}

					CoTaskMemFree(pProps->pszwName);
					CoTaskMemFree(pProps->pszwDeviceName);
					CoTaskMemFree(pProps);
				}

				pConn->Release();
				pConn = NULL;
			}

		} while (SUCCEEDED(hrT) && 1 == ulCount && !fFound);

		if (FAILED(hrT))
		{
			hr = hrT;
		}

		pEnumNetConnection->Release();
	}

	if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_RETRY))
	{
		//wprintf(L"無法啟用或禁用連結 (0x%08x)\r\n", hr);
	}

	pNetConnectionManager->Release();
	CoUninitialize();

	return hr;
}