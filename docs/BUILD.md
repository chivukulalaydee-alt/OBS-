# 构建说明

## 依赖

- Windows 10/11 x64
- Visual Studio Build Tools 2022
- Windows SDK 10.0.26100.0
- CMake
- 7-Zip
- NSIS 3.12

## 1. 获取官方 OBS 载荷

下载 OBS Studio 32.2.1 Windows x64 ZIP：

`https://github.com/obsproject/obs-studio/releases/download/32.2.1/OBS-Studio-32.2.1-Windows-x64.zip`

官方 SHA-256：

`db64a2934f8261f85b1410b84be011207a0afda5400d008289f1f1e211bcc7de`

将 ZIP 解压到仓库的 `build/payload`。发布构建排除 `*.pdb` 调试符号。

## 2. 构建插件

```powershell
Set-Location plugin\media-playlist-source
cmake --preset windows-x64
cmake --build --preset windows-x64
```

将生成的 `media-playlist-source.dll` 复制到：

`build/payload/obs-plugins/64bit/media-playlist-source.dll`

将 `plugin/media-playlist-source/data` 的内容复制到：

`build/payload/data/obs-plugins/media-playlist-source`

## 3. 加入许可与源码

在 `build/payload/licenses` 中至少包含：

- OBS Studio GPL-2.0-or-later 完整文本。
- Media Playlist Source GPL-2.0 完整文本。
- OBS Studio 32.2.1 对应源码或明确的同版本源码获取方式。
- 包含 `zh-CN.ini` 本地改动的 Media Playlist Source 对应源码。

OBS Studio 32.2.1 官方源码：

`https://github.com/obsproject/obs-studio/releases/download/32.2.1/OBS-Studio-32.2.1-Sources.tar.gz`

官方 SHA-256：

`6a2532b1094bc51bc2fdeb1068d5c19cfe04216191a5b35c8707625401a80bf4`

## 4. 编译安装器

```powershell
New-Item -ItemType Directory -Force dist
& 'C:\Program Files (x86)\NSIS\makensis.exe' /INPUTCHARSET UTF8 installer\installer.nsi
```

输出文件：

`dist/OBS-Studio-32.2.1-Playlist-Edition-Setup.exe`

## 5. 最低验证门禁

1. OBS 运行时安装器必须阻止覆盖。
2. 隔离安装后的 `obs64.exe --version` 必须为 32.2.1。
3. OBS 日志必须包含 Media Playlist Source 0.1.3 加载成功记录。
4. 插件 DLL、`zh-CN.ini`、许可证和源码文件必须存在。
5. 桌面/开始菜单快捷方式目标和工作目录必须正确。
6. 卸载必须删除程序与快捷方式，但不得删除 `%APPDATA%/obs-studio`。

