<div align="center">

<img alt="LOGO" src="https://www.lovesa.cc/static/image/common/stoneii.png" width="256" height="256" />

# SaSH

<br>
<div>
    <img alt="C++" src="https://img.shields.io/badge/c++-23-%2300599C?logo=cplusplus">
</div>
<div>
    <img alt="platform" src="https://img.shields.io/badge/platform-Windows-blueviolet">
</div>
<div>
    <img alt="license" src="https://img.shields.io/github/license/bestkakkoii/SaSH">
    <img alt="commit" src="https://img.shields.io/github/commit-activity/m/bestkakkoii/SaSH?color=%23ff69b4">
    <img alt="stars" src="https://img.shields.io/github/stars/bestkakkoii/SaSH?style=social">
</div>
<br>


StoneAge Supreme Helper(SaSH) is an assistant application for you to play StoneAge8001 with highly automated game experience. 石器助手(SaSH)是一個用於遊戲石器時代8001端的工具，讓你擁有高度自動化的遊戲體驗。

</div>

### Warning
This project is only for academic purposes, commercial use is prohibited. You are prohibited to publish this project elsewhere. However we make no promises to your game accounts and so you have to use this project at your own risk, including taking any damage to your accounts from scripts and binaries.

### 特別聲明
本項目僅供學習交流，禁止用於商業用途。本項目內的所有資源文件和程序，禁止在 GitHub/lovesa論壇 以外的任何地方進行轉載或發布。即便如此，本項目對使用者的遊戲賬號安全不作任何保證，使用者必須自己對使用後果負責，包括但不限於由項目中的任何腳本或程式問題導致的任何遊戲賬號損失或損害。

---

### Project Build Requirement 項目依賴

- Visual Studio 2022(V17.6.8)
- Qt Visual Studio Tools 3.0.1(Plugin)
- VC143 Toolset
- QT 6.7.0 for msvc2019, win32 (x64)
- Windows SDK 11 (the msvc projects uses Windows SDK version 10.0.22621.0, please redirect to your local version if you have installed a different version of Windows SDK)

---

### Libs/Modules/Headers Requirement 其他依賴庫/模組/頭文件

**Qt Modules Qt模組**

Gui, Widgets, Concurrent, Core, OpenGL, Network, Concurrent

**Libs 靜態庫**

ws2_32.lib, detours.lib

**3rd Party 第三方依賴組件**

Glu32.lib, OpenGL32.lib, cpr.lib, libcurl.lib(libcurl.dll), qscintilla2_qt5.lib

---

**Gernel Setting 一般設置**

(/std:c++latest)<br>
(/std:c17)<br>
(/utf-8)<br>
(/experimental:module)<br>

**Default CodePage 預設編碼頁**

UTF-8 with BOM

---

### Common Modification Steps for Features

#### Add New Script Commands

1. For core commands, add corresponding enumeration values in lexer.h. For general commands, add corresponding traditional and simplified Chinese command mappings to `static const QHash<QString, RESERVE> keywords` in script/lexer.cpp.

2. In script/interpreter.h, add the declaration of corresponding functions, e.g., `qint64 sleep(qint64 currentline, const TokenMap& TK);` . Register the corresponding functions in openLibsBIG5, openLibsGB2312, and openLibsUTF8 in script/interpreter.cpp.

3. In a file under the script category, define the content of the new command.

- When defining a function for a new command, always use functions like `checkString`, `checkInteger`, `toVariant`, and `checkRange` to access parameters through parameter indices.

- For conditional jumps, use `checkJump` at the end of the function to correctly or incorrectly jump based on the result. Use the enumeration values `SuccessJump` or `FailedJump` as the last parameter. For example `return checkJump(TK, 7, bret, FailedJump);` means the 7th parameter is the jump line or label, `bret` is the result (true or false), and the jump is performed only when the result is false.

- Note that core commands do not need separate registration; instead, new core commands need to be declared and defined in script/parser.h and script/parser.cpp. Add corresponding switch cases in `void Parser::processTokens()` .

#### Add New UserSetting

1. First, add the corresponding enumeration value in util.h under enum UserSetting.

2. Add the corresponding JSON key-value string mapping for file saving in `static const QHash<UserSetting, QString> user_setting_string_hash` in util.h.

3. In injector.h, add the corresponding default values to `userSetting_value_hash_`, `userSetting_enable_hash_`, or `userSetting_string_hash_` based on the setting type (integer, boolean, or string).

4. For script `set` commands, add the new settings to `qint64 Interpreter::set(qint64 currentline, const TokenMap& TK)` in script/system.cpp.

- For UI additions, place them under the form category. Try to avoid complex logic in the UI and save functionality to the above hash containers for asynchronous processing by mainthread.cpp.

#### Strict Rules to Follow

Regardless of the limitations of C++ or other factors, avoid directly handling complex logic in the UI or directly manipulating the UI from other threads. Instead, use signaldispatcher.h to handle tasks via signals and slots.

### Debug compilation issues

1. Mainland users can use the mirror to install QT, such as: .\qt-unified-windows-x64-4.6.0-online.exe --mirror http://mirrors.tuna.tsinghua.edu.cn/qt

2. When the DLL is missing, you need to decompress and copy the content in SaSH_dbg_package.7z to Debug and Release.

3. Line 266 in SaSH\SaSH\injector.cpp needs to be
     dllPath = R"(YourPath\Debug\sadll.dll)";
     Change to your own absolute path.

4. You need to put all qm translation files compiled with Linguist into SaSH\translations.

### Script syntax documentation

See [https://gitee.com/Bestkakkoii/sash/wikis](https://gitee.com/Bestkakkoii/sash/wikis) for details.

---

### 常用功能修改步驟

#### 新增腳本命令

1. 核心命令需要在lexer.h新增對應枚舉值、一般命令只需要在 script/lexer.cpp 的 `static const QHash<QString, RESERVE> keywords` 中 添加對應繁簡中英文的命令

2. 在 script/interpreter.h 增加對應的函數聲明，如:`qint64 sleep(qint64 currentline, const TokenMap& TK);` 並在 script/interpreter.cpp 中的 openLibsBIG5, openLibsGB2312, openLibsUTF8 註冊對應函數

3. script分類下的文件中挑個地方定義新命令的內容

- 定義新命令的函數中取參數一律使用 `checkString`, `checkInteger`, `toVariant`, `checkRange` 等函數透過傳入參數索引取值
- 對於跳轉 則在定義函數末尾使用 `checkJump` 正確或錯誤跳轉，使用最後一個參數輸入枚舉值 `SuccessJump` 或 `FailedJump` 操作
- 如 `return checkJump(TK, 7, bret, FailedJump);` 代表 第7個參數為跳轉行數或標記，`bret` 為結果真或假，結果為假才跳轉

- 注意，核心命令不需要另外註冊取代的是需要在 script/parser.h script/parser.cpp 中新增對應的聲明和定義，並在 `void Parser::processTokens()` 中增加對應switch項

#### 人物存檔新增項

1. 首先在 util.h 中的 `enum UserSetting` 新增對應枚舉值

2. 在 util.h 中的 `static const QHash<UserSetting, QString> user_setting_string_hash` 中新增對應json存檔所用的鍵值字符串映射

3. 在 injector.h 中的 `userSetting_value_hash_`, `userSetting_enable_hash_` 或 `userSetting_string_hash_` 中心增對應預設值，取決於設置類型 分別為整數、布爾或字符串三種

4. 對於腳本 `set` 命令則須要到 script/system.cpp 的 `qint64 Interpreter::set(qint64 currentline, const TokenMap& TK)` 中新增

5. UI新增一律都在 form 分類中，所有的功能邏輯盡可能不在UI內執行，而是保存到上述的 hash容器中交由 mainthread.cpp 異步處理

#### 嚴守的規則

不管是基於C++本身的限制也好，或其他因素，嚴禁在UI內直接處理太過複雜的邏輯，或在其他線程直接操作UI，請一律交由 signaldispatcher.h 透過信號槽處理

### 調試編譯問題

1. 大陸用戶可使用鏡像安裝QT，如： .\qt-unified-windows-x64-4.6.0-online.exe --mirror http://mirrors.tuna.tsinghua.edu.cn/qt

2. 缺少DLL時，需要將 bin 中的內容拷貝到 Debug 或 Release 下。

3. 需要將 `SaSH\SaSH\injector.cpp` 中 `266行` `dllPath = R"(YourPath\Debug\sadll.dll)";` 改為自己的絕對路徑。

4. 需要將 使用Linguist編譯好的qm翻譯文件放到 SaSH\translations 中。

5. 建置後出現找不到bind7z.bat請打開項目設置->建置事件->建置後事件，將 命令列 中的內容移除

6. 首次編譯後請打開 `Qt 6.7.0 (MSVC 2019 64-bit)` cd 到 Relase/Debug 後輸入 `windeployqt SaSH.exe`

### 腳本語法說明文檔

詳見 [https://gitee.com/Bestkakkoii/sash/wikis](https://gitee.com/Bestkakkoii/sash/wikis)。

### 貢獻/參與者

感謝所有參與到開發/測試中的朋友們，大家的幫助讓 SaSH 越來越好！ (\*´▽｀)ノノ

[![Contributors](https://contrib.rocks/image?repo=bestkakkoii/SaSH)](https://github.com/bestkakkoii/SaSH/graphs/contributors)

## 广告

用户交流 QQ 群：[石器助手SaSH](https://qm.qq.com/cgi-bin/qm/qr?k=Z9Tf9t4AS4BG12sVB_SUPV88rJGrwMXW&jump_from=webapi&authKey=lYhKc6dK6bdCoFgEaPe4wQGjhcN9i79/BPkhP5oHcUIY7nr5LYiKS0cRsZhR31yi)<br>

如果覺得SaSH對你有幫助，幫忙點個 Star 吧！～（github網頁最上方右上角的小星星），這就是對我們最大的支持了！