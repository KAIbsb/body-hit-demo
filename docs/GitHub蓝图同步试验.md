# GitHub 蓝图同步预检

日期：2026-10-12。按用户最新要求，本轮不提交、不上传当前工程，也不创建远程仓库。

## 当前结论

本地 Git 与 Git LFS 可以完整存储和还原本项目的蓝图与关卡文件，还原副本能在 UE 5.8.3 中加载、编译。GitHub 账号与只读 Git 连接已确认可用。

**GitHub 上的 `.uasset` 上传、提交、拉取仍未验证，暂不视为 GitHub 协作验收完成。** 最终允许测试提交后，需要在独立测试分支上传样本、从另一份克隆拉取、核对 SHA-256 并在 UE 加载，再决定正式使用。

## 本轮实际检查

| 检查 | 结果 |
|---|---|
| HUD 蓝图 `WBP_InjuryHUD.uasset` | 94,584 字节，普通 Git 与 LFS 还原均逐字节一致 |
| 角色蓝图 `BP_InjuryCharacter.uasset` | 25,534 字节，普通 Git 与 LFS 还原均逐字节一致 |
| 关卡 `L_InjuryLab.umap` | 19,205 字节，普通 Git 与 LFS 还原均逐字节一致 |
| UE 加载还原文件 | 隔离副本工程中三份资产全部加载，两份蓝图编译成功，0 errors |
| 提交与上传 | 0 次 commit；没有远程地址；未 push 或创建仓库 |
| 本机 GitHub 账号 | Git Credential Manager 中的 `RANyrrrrr`，只读 API 验证通过 |
| GitHub Git 连接 | 指定本机系统代理后 `ls-remote` 成功 |
| 二进制自动合并 | Git 文本合并器明确拒绝 `.uasset`：Cannot merge binary files |

注意：本机 GitHub 直连 443 端口失败。系统代理为 `http://127.0.0.1:7892`；本轮只在单次命令中指定代理，没有修改全局 Git 配置。未来同步前应重新检查本机代理端口。

## 协作规则

- `.uasset`、`.umap` 用 Git LFS 管理；LFS 用指针记录大文件，文件本体独立存储。[GitHub 官方说明](https://docs.github.com/en/repositories/working-with-files/managing-large-files/about-git-large-file-storage)
- 同一份蓝图或关卡由一人负责编辑，其他人编辑不同资产。LFS 锁定可用于登记正在编辑的文件；它不负责自动合并蓝图逻辑。[Git LFS 锁定文档](https://github.com/git-lfs/git-lfs/blob/main/docs/man/git-lfs-lock.adoc)
- 正式工程应包含 `.uproject`、`Config`、`Content`、`Source` 和所需插件；排除 `Saved`、`Intermediate`、`DerivedDataCache`、生成的 `Binaries`。需编译本项目 C++ 模块。
- 每次交接附修改说明、使用说明、注意事项；明确 UE 5.8.3、关卡/蓝图位置、已知问题与交接确认栏。

测试配置仅位于 `Saved/Verification/GitAssetProbe-*`，还原工程为其中的 `RestoredDemo`。这些副本和测试产物不属于正式交付工程。

证据：`Saved/Verification/GitAssetProbe.json`、`GitRestoredUE.log`。脚本：`Scripts/ProbeGitAssets.py`、`VerifyGitRestoredAssets.py`。
