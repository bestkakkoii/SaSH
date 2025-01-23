/*
                GNU GENERAL PUBLIC LICENSE
                   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2024 All Rights Reserved.
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

#ifndef CXFILEDIALOG_H
//@隱藏{
#define CXFILEDIALOG_H
//@隱藏}

//本來是寫給炫語言模塊的，移植來Qt使用

//@src "cxfiledialog.cpp"

#include <shobjidl.h> 
#include <QVector>

//@隱藏{
namespace cx
{
    //@隱藏}

    //@分組{ 炫::文件對話框(cx::file_dialog)

    //@啟用枚舉前綴

    //@別名 文件對話框接受模式
    enum FileDialogAcceptMode
    {
        //@備註 打開文件
        OpenFile = 0, //@別名 打開文件

        //@備註 打開目錄
        OpenDirectory, //@別名 打開文件夾

        //@備註 保存文件
        SaveFile, //@別名 保存文件
    };

    //@別名 文件對話框標籤
    enum FileDialogLabel
    {
        //@備註 確定按鈕標簽
        OkButton, //@別名 確定按鈕

        //@備註 文件名標簽
        FileName, //@別名 文件名
    };

    //@別名 文件對話框選項
    enum FileDialogOption
    {
        //@備註 (0x2) 啟用覆蓋文件時的提示，如果用戶選擇一個已經存在的文件
        OverwritePrompt = FOS_OVERWRITEPROMPT, //@別名 啟用覆蓋文件提示

        //@備註 (0x4) 僅顯示在文件類型篩選器中指定的文件類型，不顯示所有文件
        StrictFileTypes = FOS_STRICTFILETYPES, //@別名 嚴格文件類型

        //@備註 (0x8) 不允許改變當前目錄
        NoChangeDirectory = FOS_NOCHANGEDIR, //@別名 禁止改變目錄

        //@備註 (0x20) 允許用戶選擇文件夾而不是文件
        PickFolders = FOS_PICKFOLDERS, //@別名 允許選擇文件夾

        //@備註 (0x40) 強制使用文件系統對話框，而不是自定義對話框
        ForceFileSystem = FOS_FORCEFILESYSTEM, //@別名 強制文件系統對話框

        //@備註 (0x80) 允許選擇所有非存儲項目，不僅限於文件系統項目
        AllNonStorageItems = FOS_ALLNONSTORAGEITEMS, //@別名 允許所有非存儲項

        //@備註 (0x100) 不驗證用戶輸入的文件名或路徑
        NoValidate = FOS_NOVALIDATE, //@別名 禁止驗證

        //@備註 (0x200) 允許用戶選擇多個文件
        AllowMultiSelect = FOS_ALLOWMULTISELECT, //@別名 允許多選

        //@備註 (0x800) 路徑必須存在
        PathMustExist = FOS_PATHMUSTEXIST, //@別名 路徑必須存在

        //@備註 (0x1000) 文件必須存在
        FileMustExist = FOS_FILEMUSTEXIST, //@別名 文件必須存在

        //@備註 (0x2000) 顯示創建文件的提示，當用戶指定的文件不存在時
        CreatePrompt = FOS_CREATEPROMPT, //@別名 創建文件提示

        //@備註 (0x4000) 文件對話框應該檢查是否有其他應用程序已經打開了所選文件
        ShareAware = FOS_SHAREAWARE, //@別名 共享感知

        //@備註 (0x8000) 不允許選擇只讀文件
        NoReadOnlyReturn = FOS_NOREADONLYRETURN, //@別名 禁止只讀返回

        //@備註 (0x10000) 不測試文件的創建能力
        NoTestFileCreate = FOS_NOTESTFILECREATE, //@別名 禁止測試文件創建

        //@備註 (0x20000) 隱藏最近訪問的位置
        HideMRUPlaces = FOS_HIDEMRUPLACES, //@別名 隱藏最近訪問位置

        //@備註 (0x40000) 隱藏固定的位置
        HidePinnedPlaces = FOS_HIDEPINNEDPLACES, //@別名 隱藏固定位置

        //@備註 (0x100000) 不解析shell鏈接
        NoDereferenceLinks = FOS_NODEREFERENCELINKS, //@別名 禁止解析鏈接

        //@備註 (0x200000) 需要與確定文件名按鈕進行交互
        OkButtonNeedsInteraction = FOS_OKBUTTONNEEDSINTERACTION, //@別名 需要確定文件名按鈕交互

        //@備註 (0x2000000) 不將文件添加到最近文件列表中
        DontAddToRecent = FOS_DONTADDTORECENT, //@別名 不添加到最近文件列表

        //@備註 (0x10000000) 強制顯示隱藏文件和文件夾
        ForceShowHidden = FOS_FORCESHOWHIDDEN, //@別名 強制顯示隱藏文件

        //@備註 (0x20000000) 默認情況下不以最小模式打開
        DefaultNoMinimode = FOS_DEFAULTNOMINIMODE, //@別名 默認不以最小模式打開

        //@備註 (0x40000000) 強制顯示預覽面板
        ForcePreviewPaneOn = FOS_FORCEPREVIEWPANEON, //@別名 強制顯示預覽面板

        //@備註 (0x80000000) 支持可流式傳輸的項目
        SupportStreamableItems = FOS_SUPPORTSTREAMABLEITEMS, //@別名 支持可流式傳輸項目

        //@備註 默認打開文件
        // 覆蓋提示(0x2) | 嚴格文件類型(0x4) | 路徑必須存在(0x8000) 的組合
        DefaultOpenFile = FOS_OVERWRITEPROMPT | FOS_STRICTFILETYPES | FOS_PATHMUSTEXIST, //@別名 默認打開文件

        //@備註 默認保存文件
        // 覆蓋提示(0x2) | 文件必須存在(0x1000) | 嚴格文件類型(0x4) | 路徑必須存在(0x8000) 的組合
        DefaultSaveFile = FOS_OVERWRITEPROMPT | FOS_FILEMUSTEXIST | FOS_STRICTFILETYPES | FOS_PATHMUSTEXIST, //@別名 默認保存文件

        //@備註 默認打開文件夾，包括文件夾選擇和路徑必須存在
        // 允許選擇文件夾(0x20) | 路徑必須存在(0x8000) 的組合
        DefaultOpenDirectory = FOS_PICKFOLDERS | FOS_PATHMUSTEXIST //@別名 默認打開文件夾
    };

    //@禁用枚舉前綴

    //@別名 文件對話框
    class file_dialog
    {

    public:
        //@隱藏{
        Q_DECLARE_FLAGS(FileDialogOptions, FileDialogOption);
        //@隱藏}

#if 0
        file_dialog(HWND parent, cx::WindowFlags flags);
#endif

        //@備注 構造
        //@參數 選項屬性枚舉值組合
        //@參數 父窗口句柄(選填)
        //@參數 標題(選填)
        //@參數 目錄(選填)
        //@返回 無
        //@別名 構造(選項, 父窗口, 標題, 目錄)
        explicit file_dialog(FileDialogAcceptMode mode,
            HWND parent = nullptr,
            const QString& caption = "",
            const QString& directory = "");

        //@備注 構造
        //@參數 選項屬性枚舉值組合
        //@參數 父窗口句柄(選填)
        //@返回 無
        //@別名 構造(選項, 父窗口)
        file_dialog(qint64 mode, HWND parent);

        //@備注 析構
        //@別名 析構
        virtual	~file_dialog();

        //@備注 是否有效
        //@返回 true 有效, false 無效
        //@別名 是否有效()
        bool isValid() const { return pDialog_ != nullptr; }

        //@備注 執行
        //@返回 true 成功, false 失敗
        //@別名 執行()
        bool exec();

        //@備注 獲取父窗口句柄
        //@返回 父窗口句柄
        //@別名 取父窗口句柄()
        HWND parent() const { return parent_; }

        //@備注 獲取目錄
        //@返回 目錄
        //@別名 取目錄()
        QString directory() const { return directory_; }

        //@備注 獲取文件模式
        //@返回 文件模式
        //@別名 取模式()
        FileDialogAcceptMode fileMode() const { return fileMode_; }

        //@備注 獲取歷史紀錄
        //@返回 歷史紀錄
        //@別名 取紀錄()
        QStringList history() const
        {
            return history_;
        }

        //@備注 獲取過濾器 
        //@返回 過濾器
        //@別名 取過濾器()
        QStringList nameFilters() const
        {
            return nameFilters_;
        }

        //@備注 獲取選項
        //@返回 選項
        //@別名 取選項()
        FileDialogOptions options() const { return options_; }

        //@備注 設置過濾器 格式: "文本文件|*.txt" 或 "所有文件|*.*"
        //@參數 單個過濾器
        //@返回 無
        //@別名 設置過濾器()
        void setFilter(const QString& filter) { nameFilters_.clear(); nameFilters_.append(filter); }

        //@備注 設置過濾器 格式: { "文本文件|*.txt", "所有文件|*.*" }
        //@參數 多個過濾器
        //@返回 無
        //@別名 設置過濾器()
        void setFilter(const QStringList& filters)
        {
            nameFilters_ = filters;
        }

        //@備注 獲取已選擇的文件或目錄
        //@返回 已選擇的文件或目錄
        //@別名 取已選擇()
        QStringList selectedFiles() const
        {
            return selectedFiles_;
        }

        //@備注 設置起始目錄
        //@參數 目錄
        //@返回 無
        //@別名 設置目錄(目錄路徑)
        void setDirectory(const QString& directory) { directory_ = directory; }

        //@備注 設置文件模式
        //@參數 文件模式
        //@返回 無
        //@別名 設置模式(文件模式)
        void setFileMode(FileDialogAcceptMode mode) { fileMode_ = mode; }

        //@備注 設置選項
        //@參數 單個選項
        //@參數 是否啟用(選填:默認為true)
        //@返回 無
        //@別名 設置選項(選項, 是否啟用)
        void setOption(FileDialogOption option, bool on = true);

        //@備注 設置選項
        //@參數 多個選項按位或運算組合
        //@返回 無
        //@別名 設置選項s(選項)
        void setOptions(FileDialogOptions options);

        //@備注 設置窗口標題
        //@參數 標題
        //@返回 無
        //@別名 設置標題(標題)
        void setWindowTitle(const QString& title);

        //@備注 設置預設文件名
        //@參數 文件名
        //@返回 無
        //@別名 設置文件名(文件名)
        void setDefaultFileName(const QString& filename);

        //@備注 設置預設目錄
        //@參數 目錄
        //@返回 無
        //@別名 設置目錄(目錄路徑)
        void setDefaultFolder(const QString& folder);

        //@備注 設置標籤文字
        //@參數 標籤類別
        //@參數 文字
        //@返回 無
        //@別名 設置標籤文字(標籤, 文字)
        void setLabelText(FileDialogLabel label, const QString& text);

        //@備註 設置要保存的文件的初始位置。
        // 使用該方法可以將用戶打開的文件的路徑設置為保存對話框的初始位置。
        // 這可以用於讓用戶選擇要覆蓋的文件或指定默認保存位置。
        //@參數 filePath 要保存的文件的路徑。
        //@返回 無
        //@別名 設置另存為項目(文件路徑)
        void setSaveAsItem(const QString& filePath);

        //@備註 設置默認擴展名
        //@參數 擴展名
        //@返回 無
        //@別名 設置默認擴展名(擴展名)
        void setDefaultExtension(const QString& extension);

        //@隱藏{
#if 0
        //@備注 尚未實現
        //@返回 窗口句柄
        //@別名 取窗口句柄
        HWND getWindowHandle() const { return hWnd_; }

        //@備注 設置圖標 (尚未實現)
        //@參數 圖標路徑
        //@返回 無
        //@別名 設置圖標(圖標路徑)
        void setIcon(const QString& iconPath);

        //@備注 設置窗口屬性(尚未實現)
        //@參數 窗口屬性
        //@返回 無
        //@別名 設置窗口屬性(窗口屬性)
        void setWindowsFlags(WindowFlags flags) { flags_ = flags; }
#endif

    private:
        HRESULT create();

        HRESULT setNameFilters();

#if 0
        void getDialogHWND();

        void applyWindowFlags();
#endif

        HRESULT handleFilesOrDirectoriesSelection();

        HRESULT handleSaveFileSelection();

        void setDialog(IFileDialog* pDialog) { pDialog_ = pDialog; }

        void clear();

    private:
        IFileDialog* pDialog_ = nullptr;
        IFileOpenDialog* pOpenDialog_ = nullptr;
        IFileSaveDialog* pSaveDialog_ = nullptr;
        HWND parent_ = nullptr;

        FileDialogAcceptMode fileMode_ = FileDialogAcceptMode::OpenFile;
        FileDialogOptions options_ = FileDialogOption::DefaultOpenFile;

        QString directory_;
        QString caption_;
        QStringList history_;
        QStringList nameFilters_ = { "Any Files|*.*" };
        QStringList selectedFiles_;

#if 0
        cx::WindowFlags flags_ = cx::WindowType::Default;
        HWND hWnd_ = nullptr;
        HICON icon_ = nullptr;
#endif
        //@隱藏}
    };

#if 0
    //@備註 創建打開文件對話框
    //@參數 目錄 起始目錄(選填)
    //@參數 標題 對話框標題(選填)
    //@參數 父窗口 父窗口句柄(選填)
    //@返回 已選擇的文件
    //@別名 創建打開文件對話框(目錄, 標題, 父窗口)
    static QStringList createOpenFileDialog(const QString& directory = "", const QString& caption = "", HWND parent = nullptr)
    {
        static QStringList empty;
        file_dialog dialog(FileDialogAcceptMode::OpenFile, parent, caption, directory);
        if (dialog.exec())
            return dialog.selectedFiles();
        return empty;
    }

    //@備註 創建打開文件夾對話框
    //@參數 目錄 起始目錄(選填)
    //@參數 標題 對話框標題(選填)
    //@參數 父窗口 父窗口句柄(選填)
    //@返回 已選擇的目錄
    //@別名 創建打開文件夾對話框(目錄, 標題, 父窗口)
    static QStringList createOpenDirectoryDialog(const QString& directory = "", const QString& caption = "", HWND parent = nullptr)
    {
        static QStringList empty;
        file_dialog dialog(FileDialogAcceptMode::OpenDirectory, parent, caption, directory);
        if (dialog.exec())
            return dialog.selectedFiles();
        return empty;
    }

    //@備註 創建保存文件對話框
    //@參數 目錄 起始目錄(選填)
    //@參數 標題 對話框標題(選填)
    //@參數 父窗口 父窗口句柄(選填)
    //@返回 已選擇的文件
    //@別名 創建保存文件對話框(目錄, 標題, 父窗口)
    static QStringList createSaveFileDialog(const QString& directory = "", const QString& caption = "", HWND parent = nullptr)
    {
        static QStringList empty;
        file_dialog dialog(FileDialogAcceptMode::SaveFile, parent, caption, directory);
        if (dialog.exec())
            return dialog.selectedFiles();
        return empty;
    }
#endif

    //@分組}




    //@隱藏{
}
//@隱藏}

#endif // CXFILEDIALOG_H