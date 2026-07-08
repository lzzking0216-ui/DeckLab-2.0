# 验证命令

本项目在当前机器上的 Qt/CMake 验证路径：

- VS 环境初始化：`C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat`
- CMake：`E:\QT_software\QT\Tools\CMake_64\bin\cmake.exe`
- jom：`E:\QT_software\QT\Tools\QtCreator\bin\jom\jom.exe`
- 构建目录：`E:\DeckLab-2.0\build\Desktop_Qt_6_8_3_MSVC2022_64bit-Debug`

在仓库根目录运行：

```bat
cmd /c "call ""C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"" -arch=x64 -host_arch=x64 && set PATH=E:\QT_software\QT\Tools\QtCreator\bin\jom;%PATH% && E:\QT_software\QT\Tools\CMake_64\bin\cmake.exe --build build\Desktop_Qt_6_8_3_MSVC2022_64bit-Debug"
```

看到 `[100%] Built target DeckLab` 表示编译验证通过。