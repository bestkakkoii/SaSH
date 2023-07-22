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

---

### Project Build Requirement 項目依賴

- Visual Studio 2022(V17.6.5)
- Qt Visual Studio Tools 2.10.1.2(Plugin)
- VC143 Toolset
- QT 5.15.2 for msvc2019, win32 (x86)
- Windows SDK 11 (the msvc projects uses Windows SDK version 10.0.22621.0, please redirect to your local version if you have installed a different version of Windows SDK)

---

### Libs/Modules/Headers Requirement 其他依賴庫/模組/頭文件

**Qt Modules Qt模組**

Gui, Widgets, Concurrent, Core, OpenGL, PrintSupport, Network, Concurrent

**Libs 靜態庫**

ws2_32.lib, detours.lib

**3rd Party 第三方依賴組件**

Glu32.lib, OpenGL32.lib, cpr.lib, libcurl.lib(libcurl.dll), qscintilla2_qt5.lib

---

**Gernel Setting 一般設置**

(/std:c++17)
(/std:c17)
(/utf-8)

**Default CodePage 預設編碼頁**

UTF-8 with BOM

---

### Removed Files

The following files have been removed from the project:

- webauthenticator.h
- webauthenticator.cpp
- crypto.h
- crypto.cpp
- sadll.cpp

### Impact on Compilation

Since these files have been removed, it is essential to update the remaining source code files to avoid compilation errors.

1. **Update #include Directives:** Make sure to review all source files and remove any `#include` directives that reference the removed files. If there are dependencies that still need to be included, update the directives accordingly.

2. **Check Function and Class References:** Verify that there are no references to functions or classes that were defined in the removed files. If such references exist, either remove them if they are no longer needed or find appropriate replacements within the existing codebase.

3. **Resolve Build Errors:** After making the necessary updates, attempt to compile the project. If there are any build errors related to the removed files, address them by removing the problematic code or finding suitable replacements.

### Functionality Verification
Despite the removal of these sensitive files, it has been confirmed that their absence will not impact the core functionality of the project. All major features and functionalities remain intact.

The decision to remove these files was made to prioritize the security and protection of sensitive information within the project. The codebase has been carefully reviewed and updated to ensure that the removal has minimal impact on the overall functionality.


### Impact on functionalities

1. **Script encryption and decryption:** The ability to encrypt and decrypt scripts.

2. **Captcha OCR:** The capability to perform Optical Character Recognition on captchas for verification.

3. **Network authentication:** The process of authenticating users or devices on the network.

---

### 已移除的文件

以下文件已從項目中移除：

- webauthenticator.h
- webauthenticator.cpp
- crypto.h
- crypto.cpp
- sadll.cpp

### 對編譯的影響

由於這些文件已被移除，必須更新其餘的源代碼文件，以避免編譯錯誤。

1. **更新#include指令：** 請確保檢查所有源文件，並刪除引用已移除文件的 `#include` 指令。如有仍需要引用的依賴項，請相應更新指令。

2. **檢查函數和類別的引用：** 確認是否有引用在已移除文件中定義的函數或類別。若有此類引用，若不再需要，請移除它們，或在現有代碼庫中尋找合適的替代方案。

3. **解決編譯錯誤：** 完成必要的更新後，嘗試編譯項目。如出現與已移除文件相關的編譯錯誤，請刪除造成問題的代碼，或尋找合適的替代方案。

### 功能驗證

儘管移除這些敏感文件，經確認其缺失不會影響項目的主要功能。所有主要功能和功能都保持完好。

移除這些文件的決策是為了保障項目內敏感資訊的安全和保護。代碼庫已經經過仔細檢查和更新，以確保這些移除不會對整體功能造成太大影響。

### 影響的功能

1. **腳本加密和解密：** 無法正常進行腳本的加密和解密操作。

2. **驗證碼OCR：** 無法進行驗證碼的圖片字符識別。

3. **網絡驗證：** 無法順利地進行網絡驗證，即驗證用戶或設備的身份。

---

### 常用功能修改步驟

#### 新增腳本命令

1. 核心命令需要在lexer.h新增對應枚舉值、
一般命令只需要在 script/lexer.cpp 的 `static const QHash<QString, RESERVE> keywords` 中 添加對應繁簡中英文的命令

2. 在 script/interpreter.h 增加對應的函數聲明，如:`qint64 sleep(qint64 currentline, const TokenMap& TK);`
並在 script/interpreter.cpp 中的 openLibsBIG5, openLibsGB2312, openLibsUTF8 註冊對應函數

3. script分類下的文件中挑個地方定義新命令的內容

- 定義新命令的函數中取參數一律使用 `checkString`, `checkInteger`, `toVariant`, `checkRange` 等函數透過傳入參數索引取值
- 對於跳轉 則在定義函數末尾使用 `checkJump` 正確或錯誤跳轉，使用最後一個參數輸入枚舉值 `SuccessJump` 或 `FailedJump` 操作

- 注意，核心命令不需要另外註冊取代的是需要在 script/parser.h script/parser.cpp 中新增對應的聲明和定義，
並在 `void Parser::processTokens()` 中增加對應switch項

#### 人物存檔新增項

1. 首先在 util.h 中的 `enum UserSetting` 新增對應枚舉值

2. 在 util.h 中的 `static const QHash<UserSetting, QString> user_setting_string_hash` 中新增對應json存檔所用的鍵值字符串映射

3. 在 injector.h 中的 `userSetting_value_hash_`, `userSetting_enable_hash_` 或 `userSetting_string_hash_` 中心增對應預設值
取決於設置類型 分別為整數、布爾或字符串三種

4. 對於腳本set命令則須要到 script/system.cpp 的 `qint64 Interpreter::set(qint64 currentline, const TokenMap& TK)` 中新增

5. UI新增一律都在 form 分類中，所有的功能邏輯盡可能不在UI內執行，而是保存到上述的 hash容器中交由 mainthread.cpp 異步處理

#### 嚴守的規則

不管是基於C++本身的限制也好，或其他因素，嚴禁在UI內直接處理邏輯，或在其他線程直接操作UI，請一律交由 signaldispatcher.h 透過信號槽處理