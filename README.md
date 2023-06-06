# Sash

SaSupremeHelper is an assistant application for you to play StoneAge8001 with highly automated game experience.
石器助手是一個用於遊戲石器時代8001端的工具，讓你擁有高度自動化的遊戲體驗。
### Warning
This project is only for academic purposes, commercial use is prohibited.
You are prohibited to publish this project elsewhere.
However we make no promises to your game accounts and so you have to use this project at your own risk, including taking any damage to your accounts from scripts and binaries.
### 特別聲明 
本項目僅供學習交流，禁止用於商業用途。
本項目內的所有資源文件和程序，禁止在 GitHub/lovesa論壇 以外的任何地方進行轉載或發布。
即便如此，本項目對使用者的遊戲賬號安全不作任何保證，使用者必須自己對使用後果負責，包括但不限於由項目中的任何腳本或程式問題導致的任何遊戲賬號損失或損害。

### Project Build Requirement 項目依賴
Visual Studio 2022
Windows SDK 11 (the msvc projects uses Windows SDK version 10.0.22621.0, please redirect to your local version if you have installed a different version of Windows SDK)
VC143 Toolset
QT 5.15.2 for msvc2019, win32 (x86), with VS2022(V17.6.2) + Qt Visual Studio Tools 2.10.1.2(Plugin)


### Libs/Modules/Headers Requirement 其他依賴庫/模組/頭文件

**Qt Modules Qt模組**
Gui, Widgets, Concurrent, Core, OpenGL, PrintSupport

**Libs 靜態庫**
ws2_32.lib, detours.lib

**3rd Party 第三方依賴組件**
Glu32.lib, OpenGL32.lib, cpr.lib, libcurl.lib(libcurl.dll)


**Gernel Setting 一般設置**
(/std:c++17)
(/std:c17)
(/utf-8)

**Default CodePage 預設編碼頁**
UTF-8 with BOM