@echo off

cd D:\Users\bestkakkoii\Desktop\vs_project\SaSH\Release
copy sadll.dll D:\Users\bestkakkoii\Desktop\SaSH\sadll.dll
copy SaSH.exe D:\Users\bestkakkoii\Desktop\SaSH\SaSH.exe

cd D:\Users\bestkakkoii\Desktop\vs_project\SaSH\SaSH\translations
copy qt_zh_TW.qm D:\Users\bestkakkoii\Desktop\SaSH\translations\qt_zh_TW.qm
copy qt_zh_CN.qm D:\Users\bestkakkoii\Desktop\SaSH\translations\qt_zh_CN.qm

cd D:\Users\bestkakkoii\Desktop\vs_project\SaSH\ReleaseTool
set UPX_PATH="D:\Users\bestkakkoii\Desktop\tool\UPX\upx\402\upx32.exe"

rem Check if UPX executable path exists
if not exist %UPX_PATH% (
    echo UPX executable does not exist. Please verify the path.
    pause
    exit /b
)

rem Compress the executable files using UPX with LZMA compression
%UPX_PATH% --lzma "D:\Users\bestkakkoii\Desktop\SaSH\sadll.dll"
%UPX_PATH% --lzma "D:\Users\bestkakkoii\Desktop\SaSH\SaSH.exe"

del SaSH.7z

7z.exe a "D:\Users\bestkakkoii\Desktop\vs_project\SaSH\ReleaseTool\SaSH.7z" "D:\Users\bestkakkoii\Desktop\SaSH\*"

releaseTool.exe 198.13.52.137 SaSH.7z /update/ lovesash rhswzTMzdrGANyJe

pause
