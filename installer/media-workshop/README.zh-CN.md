# OBS 素材工作台完整安装器

本目录保存 OBS 素材工作台 1.0.1 的 Windows 图形安装器源码。安装器把已经构建完成的 OBS、素材工作台和 Media Playlist Source 打包到一个 EXE 中，用户无需预先安装官方 OBS，并可在安装界面选择目标目录。

## 目录结构

```text
installer/media-workshop/
├─ InstallerBootstrap.cs
├─ Build-Installer.ps1
└─ payload/
   ├─ install.cmd
   ├─ install.ps1
   ├─ SOURCE_NOTICE.md
   └─ obs-studio-media-workshop.zip   # 构建前自行放入，不提交 Git
```

`obs-studio-media-workshop.zip` 和生成的安装器体积较大，不进入 Git 历史。正式二进制通过 GitHub Release 分发。

## 构建要求

- Windows 10 或 Windows 11 x64。
- Windows PowerShell 5.1。
- .NET Framework 4.x 自带的 64 位 C# 编译器，默认路径：

```text
C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe
```

- 与源码版本匹配的 `payload\obs-studio-media-workshop.zip`。

构建脚本会强制校验 payload 的 SHA-256：

```text
A1F0A26C77A993B0A7B53DFDB361D61FEA75A7386BD5C7078DF9A4B5D1F900A6
```

哈希不匹配时构建会立即失败，避免把错误或被修改的 OBS 安装树装入安装器。

## 构建命令

在仓库根目录运行：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\installer\media-workshop\Build-Installer.ps1 `
  -OutputExe .\installer\media-workshop\output\OBS-Media-Workshop-1.0.1-Setup.exe
```

脚本会：

1. 检查所有必需文件。
2. 校验 OBS payload 哈希。
3. 检查 `install.ps1` 使用 UTF-8 BOM，以兼容 Windows PowerShell 5.1。
4. 生成临时 ZIP 资源。
5. 以 `winexe` 方式编译中文 WinForms 安装器，因此正常图形安装不会出现控制台黑窗口。
6. 输出最终安装器路径、大小和 SHA-256。

## 安装器行为

- 默认安装到 `%LOCALAPPDATA%\OBS素材工作台`。
- 图形界面允许用户选择其他专用目录。
- 不允许直接安装到磁盘根目录或 Windows 系统目录。
- 覆盖安装前会创建带时间戳的旧版本备份。
- 新版本换入失败时会尝试恢复旧版本。
- 安装结束后创建桌面快捷方式并可直接启动定制 OBS。
- 安装目录内提供 `卸载OBS素材工作台.cmd`。

无人值守测试参数：

```powershell
.\OBS-Media-Workshop-1.0.1-Setup.exe --yes `
  --install-root "D:\OBS 素材工作台测试" `
  --no-launch `
  --log "D:\OBS 素材工作台测试\install.log"
```

## 发布

请把以下文件作为 GitHub Release 附件上传，不要使用普通 Git 提交大体积安装包：

- `OBS-Media-Workshop-1.0.1-Setup.exe`
- `使用说明.txt`
- `SHA256SUMS.txt`

正式 1.0.1 安装器 SHA-256：

```text
A26FEA2196DC1E1AC3A8D1CD35FD0E459692FD80065F5D5661EF25AC87A74A65
```

## 合规与许可证

本项目用于正常直播制作、素材离线处理、循环播放和随机播放。请确保视频、音频、字体、图片和插件拥有合法使用权，并遵守直播平台规则。不要把本项目宣传为绕过平台审核、规避识别或“防封”工具。

公开分发时必须保留 OBS Studio、Media Playlist Source、FFmpeg 及其他第三方组件的许可证和源码获取说明，具体见 `payload\SOURCE_NOTICE.md`。
