# OBS Studio 播放列表版

这是一个面向 Windows x64 的 OBS Studio 整包安装项目，组合了：

- OBS Studio 32.2.1
- Media Playlist Source 0.1.3
- Media Playlist Source 简体中文翻译
- 中文 NSIS 安装/卸载界面

安装完成后，打开 OBS 即可在“来源”区域添加“媒体播放列表源”，用于循环播放、随机播放和切换本地媒体素材。

## 下载

请从 [Releases](https://github.com/chivukulalaydee-alt/OBS-/releases/latest) 下载：

`OBS-Studio-32.2.1-Playlist-Edition-Setup.exe`

下载后可以使用仓库中的 `SHA256SUMS.txt` 校验文件完整性。

## 使用方法

1. 运行安装器并完成安装。
2. 打开桌面的“OBS Studio 播放列表版”。
3. 在 OBS 的“来源”区域点击 `+`。
4. 选择“媒体播放列表源”。
5. 添加本地文件或文件夹，并按需启用“循环播放列表”和“随机播放”。

素材透明度仍使用 OBS 自带滤镜调整：右键来源，选择“滤镜”，添加“颜色校正”，调整“不透明度”。

## 功能范围

本项目用于正常直播制作中的本地媒体播放列表、循环播放、随机播放和素材切换。

本项目不包含平台风控规避、视频指纹扰动、账号登录、推流密钥、用户场景、个人素材或直播账号数据。

## 仓库结构

- `plugin/media-playlist-source`：插件 0.1.3 源码及简体中文翻译。
- `installer`：Unicode 中文 NSIS 安装脚本与许可说明。
- `docs/BUILD.md`：整包构建说明。
- `docs/VERIFICATION.md`：已执行的安装、启动和卸载验证。
- `third_party`：OBS 与插件的 GPL 许可证文本。

## 构建

构建脚本不会下载或提交 OBS 二进制。请根据 [构建说明](docs/BUILD.md) 准备 `build/payload`，然后使用 NSIS 3.12 编译：

```powershell
New-Item -ItemType Directory -Force dist
& 'C:\Program Files (x86)\NSIS\makensis.exe' /INPUTCHARSET UTF8 installer\installer.nsi
```

## 许可证与上游

- OBS Studio：[obsproject/obs-studio](https://github.com/obsproject/obs-studio)，GPL-2.0-or-later。
- Media Playlist Source：[CodeYan01/media-playlist-source](https://github.com/CodeYan01/media-playlist-source)，GPL-2.0。
- 本仓库保留完整许可证文本、上游地址和对应源码说明。

安装器目前未使用商业代码签名证书，Windows SmartScreen 可能显示“未知发布者”。

