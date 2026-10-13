#include "BuildInjuryLabCommandlet.h"
#include "InjuryLabCharacter.h"
#include "InjuryLabGameMode.h"
#include "InjuryLabPlayerController.h"
#include "InjuryLabWidget.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/ComboBoxString.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "Animation/SkeletalMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "FileHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

namespace
{
bool SaveAsset(UObject* Asset)
{
    if (!Asset) return false;
    auto* P=Asset->GetOutermost(); P->MarkPackageDirty();
    const FString Filename=FPackageName::LongPackageNameToFilename(P->GetName(),FPackageName::GetAssetPackageExtension());
    FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
    return UPackage::SavePackage(P,Asset,*Filename,Args);
}
UBlueprint* MakeBP(UClass* Parent,const TCHAR* PackagePath,const TCHAR* Name)
{
    if (auto* Existing=LoadObject<UBlueprint>(nullptr,*FString::Printf(TEXT("%s.%s"),PackagePath,Name))) return Existing;
    auto* B=FKismetEditorUtilities::CreateBlueprint(Parent,CreatePackage(PackagePath),Name,BPTYPE_Normal);
    FAssetRegistryModule::AssetCreated(B); FKismetEditorUtilities::CompileBlueprint(B); return B;
}
UCanvasPanelSlot* Place(UCanvasPanel* Canvas,UWidget* Widget,float X,float Y,float W,float H,FVector2D Anchor=FVector2D::ZeroVector,FVector2D Align=FVector2D::ZeroVector)
{
    auto* Slot=Canvas->AddChildToCanvas(Widget); Slot->SetAnchors(FAnchors(Anchor.X,Anchor.Y)); Slot->SetAlignment(Align); Slot->SetPosition(FVector2D(X,Y)); Slot->SetSize(FVector2D(W,H)); return Slot;
}
UTextBlock* Label(UWidgetTree* Tree,UCanvasPanel* Canvas,FName Name,const TCHAR* Text,float X,float Y,float W,float H,int32 Size=18,FLinearColor Color=FLinearColor::White)
{
    auto* T=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name); T->SetText(FText::FromString(Text));
    auto Font=T->GetFont(); Font.Size=Size; T->SetFont(Font); T->SetColorAndOpacity(FSlateColor(Color));
    T->SetAutoWrapText(true); Place(Canvas,T,X,Y,W,H); return T;
}
void Button(UWidgetTree* Tree,UCanvasPanel* Canvas,FName Name,const TCHAR* Text,float X,float Y,float W=272,float H=48)
{
    auto* B=Tree->ConstructWidget<UButton>(UButton::StaticClass(),Name);
    FindFProperty<FBoolProperty>(UButton::StaticClass(),TEXT("IsFocusable"))->SetPropertyValue_InContainer(B,false);
    B->SetBackgroundColor(FLinearColor(0.10f,0.19f,0.27f,1));
    auto* T=Tree->ConstructWidget<UTextBlock>(); T->SetText(FText::FromString(Text)); auto F=T->GetFont(); F.Size=18; T->SetFont(F); T->SetColorAndOpacity(FSlateColor(FLinearColor::White)); B->AddChild(T); Place(Canvas,B,X,Y,W,H);
}
UCanvasPanel* Panel(UWidgetTree* Tree,UCanvasPanel* Root,FName Name,float X,float Y,float W,float H,FVector2D Anchor,FVector2D Align)
{
    auto* Border=Tree->ConstructWidget<UBorder>(); Border->SetBrushColor(FLinearColor(0.018f,0.025f,0.04f,0.9f)); Border->SetPadding(FMargin(0));
    auto* C=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),Name); Border->AddChild(C); Place(Root,Border,X,Y,W,H,Anchor,Align); return C;
}
void SetDefaultOptions(UComboBoxString* Combo,const TArray<FString>& Values)
{
    // AddOption 仅改变运行时列表；默认列表是需要序列化的 private UPROPERTY。
    auto* Property=FindFProperty<FArrayProperty>(UComboBoxString::StaticClass(),TEXT("DefaultOptions"));
    *Property->ContainerPtrToValuePtr<TArray<FString>>(Combo)=Values;
    Combo->ClearOptions(); for (const auto& Value:Values) Combo->AddOption(Value);
    Combo->SetSelectedOption(Values[0]);
    FindFProperty<FBoolProperty>(UComboBoxString::StaticClass(),TEXT("bIsFocusable"))->SetPropertyValue_InContainer(Combo,false);
}
void UpdateMinecraftHUD(UWidgetBlueprint* Blueprint)
{
    auto* Tree=Blueprint->WidgetTree.Get();
    auto* Body=Tree->FindWidget<UCanvasPanel>(TEXT("BodyPanel"));
    if (!Body) return;
    struct ButtonText { const TCHAR* Name; const TCHAR* Text; };
    const ButtonText Labels[]={
        {TEXT("HeadButton"),TEXT("头部（震撼弹）")}, {TEXT("FlashButton"),TEXT("闪光弹")},
        {TEXT("ArmButton"),TEXT("手臂中弹")}, {TEXT("TorsoButton"),TEXT("躯干中弹")},
        {TEXT("LegButton"),TEXT("腿部中弹")}
    };
    for (const auto& Entry:Labels)
        if (auto* B=Tree->FindWidget<UButton>(Entry.Name))
            if (auto* Text=Cast<UTextBlock>(B->GetChildAt(0))) Text->SetText(FText::FromString(Entry.Text));
    const TCHAR* OldParts[]={TEXT("LeftUpperArm"),TEXT("RightUpperArm"),TEXT("LeftForearm"),TEXT("RightForearm"),
        TEXT("LeftHand"),TEXT("RightHand"),TEXT("LeftThigh"),TEXT("RightThigh"),TEXT("LeftCalf"),TEXT("RightCalf"),TEXT("LeftFoot"),TEXT("RightFoot")};
    for (const auto* Name:OldParts)
    {
        const FName WidgetName(*FString::Printf(TEXT("Part_%s"),Name));
        if (auto* Part=Tree->FindWidget(WidgetName)) Tree->RemoveWidget(Part);
        if (Blueprint->WidgetVariableNameToGuidMap.Contains(WidgetName)) Blueprint->OnVariableRemoved(WidgetName);
    }
    struct Block { const TCHAR* Name; float X,Y,W,H; };
    const Block Parts[]={
        {TEXT("Part_Head"),117,78,46,46}, {TEXT("Part_Torso"),117,130,46,69},
        {TEXT("Part_RightArm"),88,130,23,69}, {TEXT("Part_LeftArm"),169,130,23,69},
        {TEXT("Part_RightLeg"),114,205,23,69}, {TEXT("Part_LeftLeg"),143,205,23,69}
    };
    for (const auto& P:Parts)
    {
        auto* Part=Tree->FindWidget<UBorder>(P.Name);
        if (!Part) Part=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),P.Name);
        Part->SetBrushColor(FLinearColor(0.38f,0.40f,0.44f,1));
        auto* Slot=Cast<UCanvasPanelSlot>(Part->Slot);
        if (!Slot || Part->GetParent()!=Body) Slot=Body->AddChildToCanvas(Part);
        Slot->SetPosition(FVector2D(P.X,P.Y)); Slot->SetSize(FVector2D(P.W,P.H));
    }
    // 保留控件以便同事继续编辑，只隐藏本次规格要求不显示的内容。
    for (const FName Name:{FName(TEXT("RecoveryPanel")),FName(TEXT("StatePanel"))})
        if (auto* PanelWidget=Tree->FindWidget(Name); PanelWidget && PanelWidget->GetParent()) PanelWidget->GetParent()->SetVisibility(ESlateVisibility::Collapsed);
    for (const FName Name:{FName(TEXT("ExperimentTitle")),FName(TEXT("InputHelp")),FName(TEXT("ArmPartCombo")),FName(TEXT("LegPartCombo")),FName(TEXT("TierText")),FName(TEXT("ModifierText"))})
        if (auto* Widget=Tree->FindWidget(Name)) Widget->SetVisibility(ESlateVisibility::Collapsed);
    auto MovePanel=[Tree](FName Name,float X,float Y,float W,float H,FVector2D Anchor,FVector2D Align)
    {
        auto* PanelWidget=Tree->FindWidget(Name);
        auto* Slot=PanelWidget && PanelWidget->GetParent() ? Cast<UCanvasPanelSlot>(PanelWidget->GetParent()->Slot) : nullptr;
        if (Slot) { Slot->SetAnchors(FAnchors(Anchor.X,Anchor.Y)); Slot->SetAlignment(Align); Slot->SetPosition(FVector2D(X,Y)); Slot->SetSize(FVector2D(W,H)); }
    };
    MovePanel(TEXT("HealthPanel"),24,24,320,88,FVector2D::ZeroVector,FVector2D::ZeroVector);
    MovePanel(TEXT("ExperimentsPanel"),24,136,220,280,FVector2D::ZeroVector,FVector2D::ZeroVector);
    MovePanel(TEXT("BodyPanel"),-24,-24,280,340,FVector2D(1,1),FVector2D(1,1));
    auto Resize=[Tree](FName Name,float X,float Y,float W,float H)
    {
        if (auto* Widget=Tree->FindWidget(Name))
            if (auto* Slot=Cast<UCanvasPanelSlot>(Widget->Slot)) { Slot->SetPosition(FVector2D(X,Y)); Slot->SetSize(FVector2D(W,H)); }
    };
    const TCHAR* ButtonNames[]={TEXT("HeadButton"),TEXT("FlashButton"),TEXT("ArmButton"),TEXT("TorsoButton"),TEXT("LegButton")};
    for (int32 I=0;I<5;++I) Resize(ButtonNames[I],16,12+52*I,188,44);
    Resize(TEXT("HealthText"),18,12,284,28); Resize(TEXT("HealthBar"),18,50,284,16);
    Resize(TEXT("BodyTitle"),18,14,244,28); Resize(TEXT("BodyHint"),18,46,244,24);
    if (auto* Text=Tree->FindWidget<UTextBlock>(TEXT("HealthText"))) { Text->SetText(FText::FromString(TEXT("HP  100 / 100"))); auto Font=Text->GetFont(); Font.Size=20; Text->SetFont(Font); }
    if (auto* Text=Tree->FindWidget<UTextBlock>(TEXT("BodyHint"))) Text->SetText(FText::FromString(TEXT("灰色：正常   红色：受损")));
    if (!Tree->FindWidget(TEXT("HeadStateText"))) Label(Tree,Body,TEXT("HeadStateText"),TEXT("头部：正常"),18,296,244,26,16);
    // UE 5.8 已编译蓝图添加控件时，还需要注册供蓝图引用使用的 GUID。
    Tree->ForEachWidget([Blueprint](UWidget* Widget) {
        if (!Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName())) Blueprint->OnVariableAdded(Widget->GetFName());
    });
}
UWidgetBlueprint* BuildWidget(bool bUpdateMinecraftHUD)
{
    // 已生成资产不覆盖，防止同事的 UMG 修改在再次运行命令时丢失。
    if (auto* Existing=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/InjuryLab/UI/WBP_InjuryHUD.WBP_InjuryHUD")))
    {
        // 只补齐缺失的选项，不重建设计器布局。
        auto* Arm=Existing->WidgetTree->FindWidget<UComboBoxString>(TEXT("ArmPartCombo"));
        auto* Leg=Existing->WidgetTree->FindWidget<UComboBoxString>(TEXT("LegPartCombo"));
        if (Arm && Arm->GetOptionCount()==0) SetDefaultOptions(Arm,{TEXT("左上臂"),TEXT("右上臂"),TEXT("左前臂"),TEXT("右前臂"),TEXT("左手"),TEXT("右手")});
        if (Leg && Leg->GetOptionCount()==0) SetDefaultOptions(Leg,{TEXT("左大腿"),TEXT("右大腿"),TEXT("左小腿"),TEXT("右小腿"),TEXT("左脚"),TEXT("右脚")});
        if (bUpdateMinecraftHUD) UpdateMinecraftHUD(Existing);
        FKismetEditorUtilities::CompileBlueprint(Existing); return Existing;
    }
    auto* Factory=NewObject<UWidgetBlueprintFactory>(); Factory->ParentClass=UInjuryLabWidget::StaticClass();
    auto* B=Cast<UWidgetBlueprint>(Factory->FactoryCreateNew(UWidgetBlueprint::StaticClass(),CreatePackage(TEXT("/Game/InjuryLab/UI/WBP_InjuryHUD")),TEXT("WBP_InjuryHUD"),RF_Public|RF_Standalone,nullptr,GWarn));
    auto* T=B->WidgetTree.Get();
    auto* Root=T->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("RootCanvas")); T->RootWidget=Root;
    auto* Left=Panel(T,Root,TEXT("ExperimentsPanel"),30,0,304,530,FVector2D(0,0.5f),FVector2D(0,0.5f));
    Label(T,Left,TEXT("ExperimentTitle"),TEXT("四肢受损 · 实验台"),16,16,272,36,24);
    Button(T,Left,TEXT("HeadButton"),TEXT("头部中弹"),16,62);
    Button(T,Left,TEXT("FlashButton"),TEXT("闪光弹"),16,120);
    auto* Arm=T->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(),TEXT("ArmPartCombo"));
    SetDefaultOptions(Arm,{TEXT("左上臂"),TEXT("右上臂"),TEXT("左前臂"),TEXT("右前臂"),TEXT("左手"),TEXT("右手")});
    Arm->SetSelectedIndex(0); Place(Left,Arm,16,182,272,32);
    Button(T,Left,TEXT("ArmButton"),TEXT("手臂中弹"),16,222);
    Button(T,Left,TEXT("TorsoButton"),TEXT("躯干中弹"),16,282);
    auto* Leg=T->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(),TEXT("LegPartCombo"));
    SetDefaultOptions(Leg,{TEXT("左大腿"),TEXT("右大腿"),TEXT("左小腿"),TEXT("右小腿"),TEXT("左脚"),TEXT("右脚")});
    Leg->SetSelectedIndex(0); Place(Left,Leg,16,344,272,32);
    Button(T,Left,TEXT("LegButton"),TEXT("腿部中弹"),16,384);
    Label(T,Left,TEXT("InputHelp"),TEXT("Tab 切换鼠标 / 视角\nWASD 移动 · 空格跳跃"),16,452,272,65,16,FLinearColor(0.67f,0.75f,0.83f,1));
    auto* Bottom=Panel(T,Root,TEXT("RecoveryPanel"),30,-24,304,156,FVector2D(0,1),FVector2D(0,1));
    Button(T,Bottom,TEXT("Damage25Button"),TEXT("扣 25 血 · 测试血丝接口"),16,16);
    Button(T,Bottom,TEXT("ResetButton"),TEXT("恢复满血 / 清除受伤"),16,76);
    auto* Health=Panel(T,Root,TEXT("HealthPanel"),-30,24,350,146,FVector2D(1,0),FVector2D(1,0));
    Label(T,Health,TEXT("HealthText"),TEXT("人物总血量  100 / 100"),18,16,314,34,24);
    auto* HP=T->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(),TEXT("HealthBar")); HP->SetPercent(1); HP->SetFillColorAndOpacity(FLinearColor(0.28f,0.85f,0.61f,1)); Place(Health,HP,18,62,314,14);
    Label(T,Health,TEXT("TierText"),TEXT("血丝接口档位：0 / 4"),18,94,314,28,18);
    auto* Body=Panel(T,Root,TEXT("BodyPanel"),-30,-24,350,500,FVector2D(1,1),FVector2D(1,1));
    Label(T,Body,TEXT("BodyTitle"),TEXT("身体分区"),18,14,314,30,22);
    Label(T,Body,TEXT("BodyHint"),TEXT("灰色：健康     红色：中弹"),18,48,314,25,16,FLinearColor(0.67f,0.75f,0.83f,1));
    struct Segment { const TCHAR* Name; float X,Y,W,H; };
    // 正面小人：人物左侧显示于画面右侧。六块分别为头、躯干、双臂、双腿。
    const Segment Parts[]={
        {TEXT("Part_Head"),143,82,64,64}, {TEXT("Part_Torso"),143,152,64,96},
        {TEXT("Part_RightArm"),105,152,32,96}, {TEXT("Part_LeftArm"),213,152,32,96},
        {TEXT("Part_RightLeg"),140,254,32,96}, {TEXT("Part_LeftLeg"),178,254,32,96}
    };
    for (const auto& P:Parts)
    {
        auto* Border=T->ConstructWidget<UBorder>(UBorder::StaticClass(),P.Name); Border->SetBrushColor(FLinearColor(0.38f,0.40f,0.44f,1)); Place(Body,Border,P.X,P.Y,P.W,P.H);
    }
    Label(T,Body,TEXT("ModifierText"),TEXT("后坐力 ×1.00\n换弹耗时 ×1.00\n移速接口 ×1.00"),18,372,314,90,18);
    auto* Status=Panel(T,Root,TEXT("StatePanel"),0,-24,530,98,FVector2D(0.5f,1),FVector2D(0.5f,1));
    Label(T,Status,TEXT("StatusText"),TEXT("点击左侧按钮测试受伤判定"),18,12,494,28,18);
    Label(T,Status,TEXT("FlashText"),TEXT("白屏接口：待触发"),18,48,494,28,18,FLinearColor(0.67f,0.75f,0.83f,1));
    UpdateMinecraftHUD(B);
    FAssetRegistryModule::AssetCreated(B); FKismetEditorUtilities::CompileBlueprint(B); return B;
}
}
UBuildInjuryLabCommandlet::UBuildInjuryLabCommandlet()
{
    IsClient=false; IsServer=false; IsEditor=true; LogToConsole=true;
}
int32 UBuildInjuryLabCommandlet::Main(const FString& Params)
{
    auto* Widget=BuildWidget(Params.Contains(TEXT("MinecraftHUD"))); if (!SaveAsset(Widget)) return 1;
    auto* Character=MakeBP(AInjuryLabCharacter::StaticClass(),TEXT("/Game/InjuryLab/Blueprints/BP_InjuryCharacter"),TEXT("BP_InjuryCharacter"));
    auto* Controller=MakeBP(AInjuryLabPlayerController::StaticClass(),TEXT("/Game/InjuryLab/Blueprints/BP_InjuryController"),TEXT("BP_InjuryController"));
    auto* Mode=MakeBP(AInjuryLabGameMode::StaticClass(),TEXT("/Game/InjuryLab/Blueprints/BP_InjuryGameMode"),TEXT("BP_InjuryGameMode"));
    CastChecked<AInjuryLabPlayerController>(Controller->GeneratedClass->GetDefaultObject())->InjuryHUDClass=Widget->GeneratedClass;
    auto* ModeDefault=CastChecked<AInjuryLabGameMode>(Mode->GeneratedClass->GetDefaultObject());
    ModeDefault->DefaultPawnClass=Character->GeneratedClass; ModeDefault->PlayerControllerClass=Controller->GeneratedClass;
    if (!SaveAsset(Character) || !SaveAsset(Controller) || !SaveAsset(Mode)) return 2;
    if (!FPackageName::DoesPackageExist(TEXT("/Game/InjuryLab/Maps/L_InjuryLab")))
    {
        auto* W=UEditorLoadingAndSavingUtils::NewBlankMap(false); if (!W) return 3;
        W->GetWorldSettings()->DefaultGameMode=Mode->GeneratedClass;
        auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
        auto* Floor=W->SpawnActor<AStaticMeshActor>(FVector(0,0,-25),FRotator::ZeroRotator); Floor->SetActorLabel(TEXT("实验地板")); Floor->GetStaticMeshComponent()->SetStaticMesh(Cube); Floor->SetActorScale3D(FVector(40,40,0.5f));
        auto* Start=W->SpawnActor<APlayerStart>(FVector(-600,0,100),FRotator::ZeroRotator); Start->SetActorLabel(TEXT("实验出生点"));
        auto* Sun=W->SpawnActor<ADirectionalLight>(FVector(0,0,600),FRotator(-55,-30,0)); Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable); CastChecked<UDirectionalLightComponent>(Sun->GetLightComponent())->SetIntensity(5);
        auto* Sky=W->SpawnActor<ASkyLight>(); Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable); Sky->GetLightComponent()->SetIntensity(1.5f);
        auto* Display=W->SpawnActor<ASkeletalMeshActor>(FVector(100,160,0),FRotator(0,90,0)); Display->SetActorLabel(TEXT("UE自带Manny模型展示"));
        Display->GetSkeletalMeshComponent()->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")));
        if (!UEditorLoadingAndSavingUtils::SaveMap(W,TEXT("/Game/InjuryLab/Maps/L_InjuryLab"))) return 4;
    }
    UE_LOG(LogTemp,Display,TEXT("INJURY_LAB_ASSETS_READY: Editable HUD, 3 Blueprints, experiment map")); return 0;
}
