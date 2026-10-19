# 部位受击 Demo

2026-10-19 开发中版本，单机 Unreal Engine **5.8.3** 工程。

## 下载并运行

Releases 中的 `部位受击Demo_20261013_林轩宇_v0.1.0.zip` 是早期快照，不包含本次部位耐久、部位 HUD 和 FPS 受击接入。最新源码请从仓库分支获取。

从源码运行：

```powershell
git lfs install
git clone https://github.com/Lynn-0108/body-hit-demo.git
cd body-hit-demo
git lfs pull
```

安装 UE 5.8.3 和 Visual Studio 2022 C++ 游戏开发组件，切换本机引擎版本，生成 Visual Studio 项目文件并构建 `InjuryLabEditor / Development Editor / Win64`。Git 仓库不含生成的 `Binaries/`。

打开 `四肢受损.uproject`，默认地图为 `/Game/FirstPerson/Lvl_FirstPerson`，GameMode 使用 `/Game/FPSPractice/Blueprints/BP_FPSGameMode`。

## 文档入口

- [策划案](docs/策划案.md)
- [当前进度](docs/当前进度.md)
- [接口说明](docs/接口说明.md)
- [交接说明](docs/交接说明.md)
- [协作说明](docs/协作说明.md)
- `docs/archive/`：历史文档

## 演示按键

| 按键 | 功能 |
|---|---|
| 鼠标左键 | 射击 |
| 鼠标右键 | 瞄准 |
| `R` | 换弹 |
| `F` | 重置总血和全部部位耐久 |
| `Z` | 触发闪光弹状态 |
| `X` | 触发震撼弹状态 |
| `WASD` | 移动 |
| `Shift` | 冲刺 |
| `空格` | 跳跃 |

## 项目说明

- 主 HUD 是 `/Game/InjuryLab/UI/WBP_InjuryHUD`；`UI_MainUI` 已移除，弹药和准心并入 Injury HUD。
- 部位耐久默认 30，受伤阈值 30%；躯干和四肢共用，头部没有部位耐久。
- 头部没有伤口，只扣总血量；闪光/震撼只改头部状态并发出事件。
- 躯干受伤后默认每 0.5 秒扣 1 HP，最低保留 1 HP。
- 当前演示按“射击命中后对玩家自身 Injury 结算”处理；靶人结算尚未接入。
- 后端提供换弹/移速倍率和致盲透明度/曲线；实际换弹加速、腿伤移速、白屏、耳鸣仍由前端蓝图/UI 完成。
- 旧的 1–7/R 调试键和手动实验按钮已移除，输入统一走 FPS 的 Enhanced Input。
