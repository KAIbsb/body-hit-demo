"""在隔离副本工程中加载 Git/LFS 还原资产，不保存或提交工程。"""
import json
from pathlib import Path
import unreal

project = Path(__file__).resolve().parent.parent
report_path = project / 'Saved/Verification/GitAssetProbe.json'
report = json.loads(report_path.read_text(encoding='utf-8'))
expected = Path(report['workspace']) / 'RestoredDemo'
assert Path(unreal.Paths.project_dir()).resolve() == expected.resolve(), '必须在隔离副本中验证'
checks = []
for relative in (
    'InjuryLab/UI/WBP_InjuryHUD',
    'InjuryLab/Blueprints/BP_InjuryCharacter',
    'InjuryLab/Maps/L_InjuryLab',
):
    asset = unreal.load_asset('/Game/' + relative)
    assert asset is not None, relative
    if isinstance(asset, unreal.Blueprint):
        assert unreal.BlueprintEditorLibrary.compile_blueprint(asset), relative
    checks.append(dict(asset='/Game/' + relative, asset_class=asset.get_class().get_name(), loaded=True))
report['unreal_restored_assets_loaded'] = checks
report['unreal_verified'] = True
report['unreal_project'] = str(expected)
report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
unreal.log('GIT_RESTORED_ASSETS_PASS: 3 loaded, blueprints compiled, no assets saved')
