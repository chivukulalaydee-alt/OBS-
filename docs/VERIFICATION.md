# 验证结果

正式安装器版本：OBS Studio 32.2.1 + Media Playlist Source 0.1.3。

## 已通过

- NSIS 3.12 Unicode 简体中文安装器编译，无 warning、无 error。
- OBS 运行中安装阻断：返回专用阻断码，未写入测试目录。
- D 盘隔离安装：主程序、插件、中文资源、快捷方式、卸载器和注册项齐全。
- 版本验证：`OBS Studio - 32.2.1`。
- 插件加载：日志包含 `[media-playlist-source] plugin loaded successfully (version 0.1.3)`。
- 中文资源：`data/obs-plugins/media-playlist-source/locale/zh-CN.ini` 存在。
- 许可与源码：OBS/插件 GPL 文本和对应源码归档齐全。
- 无关内容排除：未包含 PDB 和其他本机第三方插件。
- 卸载回归：安装目录、快捷方式和卸载注册项移除，OBS 用户 AppData 保留。

## 安装器校验

文件：`OBS-Studio-32.2.1-Playlist-Edition-Setup.exe`

SHA-256：

`1A84A6528DF195CD150F58C20EBA063CD7194379596D5FDE45AE27A1F929E825`

## 已知提示

- 安装器当前未数字签名，Windows SmartScreen 可能显示“未知发布者”。
- 随机播放的完整媒体效果需要使用者提供实际本地素材进行人工验收。

![中文安装界面](assets/installer-zh-cn.png)

