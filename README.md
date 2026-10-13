# 部位受击 Demo

2026-10-13 当前版本快照，单机 Unreal Engine **5.8.3** 工程。

## 下载并运行

推荐到本仓库 **Releases** 下载 `部位受击Demo_20261013_林轩宇_v0.1.0.zip`，解压到普通本地文件夹。该包包含真实蓝图/关卡资产、接口手册及 Windows 已编译模块；不需要 Git LFS。先安装 UE 5.8.3，右键 `四肢受损.uproject` → Switch Unreal Engine version → 选择本机 5.8，打开后在默认 `L_InjuryLab` 地图点击运行。包内模块针对 UE 5.8.3；如果提示模块不兼容，需要安装 Visual Studio 2022 C++ 游戏开发组件并重新编译。

继续开发可使用 Git（先安装 Git LFS）：

```powershell
git lfs install
git clone https://github.com/RANyrrrrr/body-hit-demo.git
cd body-hit-demo
git lfs pull
```

Git 仓库不含生成的 Binaries。克隆后安装 UE 5.8.3 和 Visual Studio 2022 C++ 游戏开发组件，切换本机引擎版本，生成 Visual Studio 项目文件并构建 `InjuryLabEditor / Development Editor / Win64`。

私有仓库需拥有者在 Settings → Collaborators → Add people 邀请 GitHub 账号，对方接受后才能下载。请下载 Releases 中带中文名称的工程包；GitHub 自动生成的 Source code ZIP 默认可能只含 LFS 指针。

## 演示按键

| 按键 | 功能 |
|---|---|
| 1 / 7 | 闪光弹 / 震撼弹接口 |
| 2 | 躯干受损、开始非致死流血 |
| 3 / 4 | 左臂 / 右臂受损 |
| 5 / 6 | 左腿 / 右腿受损 |
| R | 重置全部状态 |
| Tab | 切换鼠标与视角 |
| WASD / 空格 | 移动 / 跳跃 |

## 接口手册与交接

- [简洁接口使用手册](docs/四肢受损系统_接口接入说明.md)
- [完整接口参考](docs/接口详细参考.md)
- [本次修改说明](docs/交接_说明文件.md)
- [使用说明](docs/交接_使用说明.md)
- [注意事项](docs/交接_注意事项.md)
- [验证记录](docs/验证记录.md)

头部无伤口，仅区分闪光/震撼。躯干默认每 0.5 秒扣 1 HP，最低 1 HP。后端提供换弹及移速倍率、致盲时长/透明度/曲线；**实际白屏、耳鸣、换弹动画和腿伤减速由效果接入方实现**。当前已有血条、部位状态 UI 和实验按钮。多人协作请分开负责蓝图/关卡，编辑前锁定对应 LFS 文件。
