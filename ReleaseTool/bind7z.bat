cd D:\Users\bestkakkoii\Desktop\vs_project\SaSH\Release
copy sadll.dll D:\Users\bestkakkoii\Desktop\SaSH\sadll.dll
copy SaSH.exe D:\Users\bestkakkoii\Desktop\SaSH\SaSH.exe

cd D:\Users\bestkakkoii\Desktop\vs_project\SaSH\SaSH\translations
copy qt_zh_TW.qm D:\Users\bestkakkoii\Desktop\SaSH\translations\qt_zh_TW.qm
copy qt_zh_CN.qm D:\Users\bestkakkoii\Desktop\SaSH\translations\qt_zh_CN.qm

cd D:\Users\bestkakkoii\Desktop\vs_project\SaSH\ReleaseTool
del SaSH.7z

7z.exe a "D:\Users\bestkakkoii\Desktop\vs_project\SaSH\ReleaseTool\SaSH.7z" "D:\Users\bestkakkoii\Desktop\SaSH\*"

releaseTool.exe 198.13.52.137 SaSH.7z /update/ lovesa fmxzmTeEkD2nBNyM

pause