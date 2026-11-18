# WinTools 启动台

个人使用的 Windows 绿色便携启动工具：把目录、应用、文件、URL 和命令行按组配置好，一键并发打开某一组。

## 功能

- 分组管理五类条目：目录、应用、文件、URL（可选浏览器）、命令行（`cmd /c` 或 `cmd /k`）。
- 按组一键并发启动，不等待进程退出；已运行的应用默认跳过，可勾选「强制重开」。
- 失效路径灰显标记，可右键「重新定位」；路径支持 `%USERPROFILE%` 等环境变量。
- 托盘常驻、关闭到托盘、托盘菜单直接打开组；单实例运行。
- 每次手动启动可选择普通 / 管理员权限模式；管理员模式下拖拽添加被 Windows 限制，界面会提示。
- 开机自启：普通模式写 HKCU Run 键，管理员模式使用计划任务（登录免 UAC）。
- 配置自动保存（`config.json`，原子写 + `.bak` 备份 + 损坏恢复），日志按天写入 `logs/`，保留最近 30 份。
- 启动结果通过托盘气泡汇总，点击气泡查看逐条明细。

## 环境

- Windows 10/11 x64
- Qt 5.12.9（MinGW 64-bit 套件）
- qmake + MinGW 构建

本机已验证的路径：

- qmake：`D:/Qt/Qt5.12.9/5.12.9/mingw73_64/bin/qmake.exe`
- MinGW：`D:/Qt/Qt5.12.9/Tools/mingw730_64/bin`

> 注意：构建前必须把 Qt 的 MinGW 目录放到 `PATH` 最前面。否则可能误用 Strawberry Perl 自带的 32 位 `g++`，链接阶段会出现大量 Qt 未定义符号。

## 构建

```bash
export PATH="/d/Qt/Qt5.12.9/Tools/mingw730_64/bin:/d/Qt/Qt5.12.9/5.12.9/mingw73_64/bin:$PATH"
mkdir -p build-release && cd build-release
qmake ../WinTools.pro
mingw32-make -j8
```

单元测试：

```bash
export PATH="/d/Qt/Qt5.12.9/Tools/mingw730_64/bin:/d/Qt/Qt5.12.9/5.12.9/mingw73_64/bin:$PATH"
mkdir -p build-tests && cd build-tests
qmake ../tests/tst_wintools.pro
mingw32-make -j8
./release/tst_wintools.exe -o results.txt,txt
```

## 部署

```bash
mkdir -p dist/WinTools
cp build-release/release/WinTools.exe dist/WinTools/
D:/Qt/Qt5.12.9/5.12.9/mingw73_64/bin/windeployqt.exe \
  --release --no-translations dist/WinTools/WinTools.exe
```

把 `dist/WinTools/` 整个目录拷到任意位置即可使用；首次运行会在同目录生成 `config.json` 与 `logs/`。

## 运行行为

- 手动双击 `WinTools.exe` 会先询问普通 / 管理员模式。
- 从托盘菜单「退出」才会真正退出；关闭窗口只会隐藏到托盘。
- 开机自启以 `--autostart` 参数启动，不弹模式对话框，直接驻留托盘。
- 命令行条目总会打开一个新的控制台窗口：未勾选「保留窗口」时命令结束即关闭（`cmd /c`），勾选后窗口保留（`cmd /k`）。

## 已知限制

- 未签名 exe 可能触发 Windows SmartScreen，选择「仍要运行」即可。
- 管理员模式下无法从资源管理器拖拽文件到窗口（Windows UIPI 限制），请使用「添加条目」。
- 程序目录移动后，开机自启中的旧路径会失效；在设置中重新开关一次自启即可重写。
- 「已运行应用」按进程 exe 路径判断；若应用由其他包装进程启动，可能无法识别。
- 命令行的引号处理依赖 `cmd.exe` 规则；包含复杂嵌套引号的命令建议先手动验证。

## 目录结构

```
src/domain/        配置模型、路径校验、命令构造、启动计划与执行（纯逻辑，可单测）
src/platform/win/  Win32 与 Qt 的平台实现（进程、提权、自启、托盘图标、单实例）
src/ui/            Qt Widgets 界面
tests/             QTest 单元测试
openspec/          需求与设计文档（变更记录）
```
