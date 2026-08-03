# 源码与许可证说明

本安装包包含用于正常直播制作的 OBS Studio 素材工作台、Media Playlist Source、FFmpeg 和 VLC/libVLC 运行库。

## 组件

- OBS Studio 32.2.1（GNU GPL v2）
  - 上游：https://github.com/obsproject/obs-studio
  - 本包定制源码：https://github.com/chivukulalaydee-alt/OBS-
- Media Playlist Source 0.1.3（GNU GPL v2）
  - 上游：https://github.com/CodeYan01/media-playlist-source
  - 本包包含简体中文翻译。
- FFmpeg 8.1.2 full build by gyan.dev（GPL v3 构建）
  - FFmpeg：https://ffmpeg.org
  - Windows 构建：https://www.gyan.dev/ffmpeg/builds/
- VLC/libVLC 3.0.23 Vetinari
  - 上游：https://github.com/videolan/vlc
  - 对应源码提交：https://github.com/videolan/vlc/tree/578d28f6c9f2379164516e689418f92ac74a3445
  - VLC 主程序及模块主要采用 GNU GPL v2 或更高版本；libVLC 库采用 GNU LGPL v2.1 或更高版本。
  - 本包仅携带 OBS“VLC 视频”来源所需的 libVLC 运行库、插件、语言及辅助数据，不包含 `vlc.exe` 桌面播放器。

各组件许可证随程序保存在 `data\obs-studio\media-workshop\licenses`；VLC 原始 `COPYING.txt` 同时保存在 `data\obs-plugins\vlc-video\runtime`。公开分发本安装包或修改后的二进制文件时，请按相应 GPL/LGPL 条款提供对应源码或有效的源码书面提供方式。
