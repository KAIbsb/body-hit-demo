"""修正实验资产的动态照明和换行，不修改同事的蓝图事件图。"""
import unreal

world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/InjuryLab/Maps/L_InjuryLab')
assert world is not None
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if isinstance(actor, unreal.DirectionalLight):
        actor.get_component_by_class(unreal.DirectionalLightComponent).set_mobility(unreal.ComponentMobility.MOVABLE)
    if isinstance(actor, unreal.SkeletalMeshActor) and actor.get_actor_label() == 'UE自带Manny模型展示':
        actor.set_actor_rotation(unreal.Rotator(pitch=0, yaw=90, roll=0), False)
assert unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/InjuryLab/Maps/L_InjuryLab')

widget = unreal.load_asset('/Game/InjuryLab/UI/WBP_InjuryHUD')
tree = unreal.find_object(widget, 'WidgetTree')
assert tree is not None, '找不到 UMG WidgetTree 子对象'
help_text = unreal.find_object(tree, 'InputHelp')
assert isinstance(help_text, unreal.TextBlock)
help_text.set_text('Tab 切换鼠标 / 视角\nWASD 移动 · 空格跳跃')
modifier_text = unreal.find_object(tree, 'ModifierText')
assert isinstance(modifier_text, unreal.TextBlock)
modifier_text.set_text('后坐力 ×1.00\n换弹耗时 ×1.00\n移速接口 ×1.00')
assert unreal.BlueprintEditorLibrary.compile_blueprint(widget)
assert unreal.EditorAssetLibrary.save_loaded_asset(widget, only_if_is_dirty=False)
unreal.log('INJURY_ASSET_POLISH_COMPLETE')
