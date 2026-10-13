"""只在 Saved 中验证 Git/LFS 二进制还原；不 commit、不建远程、不 push。"""
from pathlib import Path
import hashlib
import json
import subprocess
import uuid

project = Path(__file__).resolve().parent.parent
root = project / 'Saved' / 'Verification' / ('GitAssetProbe-' + uuid.uuid4().hex[:8])
repo = root / 'objects'
repo.mkdir(parents=True)

def git(*args, data=None, check=True):
    return subprocess.run(['git', *args], cwd=repo, input=data,
                          capture_output=True, check=check)

git('init', '--quiet', '--initial-branch=probe')
git('lfs', 'install', '--local')
(repo / '.gitattributes').write_text(
    '*.uasset filter=lfs diff=lfs merge=binary -text lockable\n'
    '*.umap filter=lfs diff=lfs merge=binary -text lockable\n', encoding='utf-8')

assets = ['Content/InjuryLab/UI/WBP_InjuryHUD.uasset',
          'Content/InjuryLab/Blueprints/BP_InjuryCharacter.uasset',
          'Content/InjuryLab/Maps/L_InjuryLab.umap']
results = []
for relative in assets:
    source = project / relative
    data = source.read_bytes()
    # 普通 Git 对象写入/读取，没有 staging 或 commit。
    oid = git('hash-object', '-w', '--stdin', data=data).stdout.decode().strip()
    regular = git('cat-file', 'blob', oid).stdout
    # LFS 指针与对象写入/还原，全程使用二进制 stdin/stdout。
    pointer = git('lfs', 'clean', relative, data=data).stdout
    restored = git('lfs', 'smudge', relative, data=pointer).stdout
    regular_path = root / 'regular' / relative
    lfs_path = root / 'lfs' / relative
    for path, content in [(regular_path, regular), (lfs_path, restored)]:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content)
    result = dict(asset=relative, bytes=len(data), sha256=hashlib.sha256(data).hexdigest(),
                  git_blob=oid, regular_identical=(regular == data),
                  lfs_identical=(restored == data),
                  lfs_pointer=pointer.decode().strip(),
                  restored_file=str(lfs_path))
    assert result['regular_identical'] and result['lfs_identical'], relative
    results.append(result)

# Git 的文本合并器遇到真实 .uasset 时会拒绝按文本合并。
asset_path = root / 'regular' / assets[0]
merge = git('merge-file', '--stdout', str(asset_path), str(asset_path), str(asset_path), check=False)
merge_message = merge.stderr.decode(errors='replace').strip()
assert merge.returncode != 0 and 'binary' in merge_message.lower(), merge_message
assert git('rev-parse', '--verify', 'HEAD', check=False).returncode != 0, '禁止产生提交'
assert git('remote').stdout == b'', '禁止配置远程'
report = dict(status='PASS', assets=results, binary_merge_message=merge_message,
              commit_created=False, remote_configured=False, uploaded=False,
              github_push_pull_tested=False, workspace=str(root))
report_path = project / 'Saved' / 'Verification' / 'GitAssetProbe.json'
report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps(dict(status=report['status'], checked_assets=len(results),
                     commits=0, uploaded=False, report=str(report_path), workspace=str(root)),
                 ensure_ascii=False))
