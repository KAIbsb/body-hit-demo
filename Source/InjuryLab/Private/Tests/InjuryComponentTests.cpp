#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "InjuryComponent.h"
#include "InjuryLabCharacter.h"
#include "InjuryLabPlayerController.h"
#include "InjuryLabWidget.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "InjuryEventProbe.h"
#include "Curves/CurveFloat.h"
#include "Components/CanvasPanelSlot.h"
#include "UObject/StrongObjectPtr.h"
namespace
{
struct FInjuryTestWorld
{
    UWorld* World;
    uint64 SimulatedFrame=GFrameCounter;
    FInjuryTestWorld()
    {
        const auto Init=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
        World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        World->InitializeActorsForPlay(FURL());
        World->BeginPlay();
        if (!World->HasBegunPlay()) World->GetWorldSettings()->NotifyBeginPlay();
    }
    ~FInjuryTestWorld() { World->EndPlay(EEndPlayReason::Quit); GEngine->DestroyWorldContext(World); World->DestroyWorld(false); }
    void Advance(float Seconds)
    {
        // 同步测试在一个编辑器帧中执行。先激活 pending timers，再模拟下一帧；还原全局计数。
        TGuardValue<uint64> FrameGuard(GFrameCounter,++SimulatedFrame);
        World->Tick(LEVELTICK_All,0.f);
        // UWorld 对超长单帧会截断 DeltaTime，因此用正常帧步进计时。
        const int32 Steps=FMath::Max(1,FMath::CeilToInt(Seconds/0.05f));
        for (int32 I=0;I<Steps;++I)
        {
            GFrameCounter=++SimulatedFrame;
            World->Tick(LEVELTICK_All,Seconds/Steps);
        }
    }
    UInjuryComponent* Component()
    {
        AActor* A=World->SpawnActor<AActor>();
        auto* C=NewObject<UInjuryComponent>(A); A->AddInstanceComponent(C); C->RegisterComponent(); return C;
    }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInjuryHealthTest,"InjuryLab.Backend.HealthAndBones",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInjuryHealthTest::RunTest(const FString&)
{
    FInjuryTestWorld Test; auto* C=Test.Component();
    TestEqual(TEXT("initial health"),C->GetHealth(),100.f);
    TestFalse(TEXT("head bullet rejects injury"),C->ApplyBulletHit(EInjuryBodyPart::Head));
    TestFalse(TEXT("head region rejects damage"),C->ApplyDamageToPart(EInjuryRegion::Head,80));
    TestEqual(TEXT("head does not alter health"),C->GetHealth(),100.f);
    TestFalse(TEXT("head has no wound"),C->IsRegionInjured(EInjuryRegion::Head));
    C->ResetInjuries();
    for (int I=1;I<=4;++I) { C->TestDamage25(); TestEqual(TEXT("25 loss tier"),C->GetBloodVisionTier(),I); }
    C->ResetInjuries();
    TestEqual(TEXT("finger maps hand"),C->ResolveBodyPart(TEXT("index_03_r")),EInjuryBodyPart::RightHand);
    TestEqual(TEXT("thigh twist maps thigh"),C->ResolveBodyPart(TEXT("thigh_twist_01_l")),EInjuryBodyPart::LeftThigh);
    TestEqual(TEXT("calf maps calf"),C->ResolveBodyPart(TEXT("calf_r")),EInjuryBodyPart::RightCalf);
    TestEqual(TEXT("ball maps foot"),C->ResolveBodyPart(TEXT("ball_l")),EInjuryBodyPart::LeftFoot);
    TestEqual(TEXT("unknown bone"),C->ResolveBodyPart(TEXT("root")),EInjuryBodyPart::Unknown);
    TestEqual(TEXT("six region mapping"),C->ResolveRegion(TEXT("hand_r")),EInjuryRegion::RightArm);
    UGameplayStatics::ApplyPointDamage(C->GetOwner(),1,FVector::ForwardVector,FHitResult(),nullptr,nullptr,nullptr);
    TestEqual(TEXT("unknown point hit does not damage torso"),C->GetHealth(),100.f);
    FHitResult Hit; Hit.BoneName=TEXT("head");
    UGameplayStatics::ApplyPointDamage(C->GetOwner(),1,FVector::ForwardVector,Hit,nullptr,nullptr,nullptr);
    TestEqual(TEXT("head point damage rejected"),C->GetHealth(),100.f);
    Hit.BoneName=TEXT("hand_r");
    UGameplayStatics::ApplyPointDamage(C->GetOwner(),12,FVector::ForwardVector,Hit,nullptr,nullptr,nullptr);
    TestEqual(TEXT("point damage amount routed once"),C->GetHealth(),88.f);
    TestTrue(TEXT("point damage preserves detailed hand state"),C->IsBodyPartInjured(EInjuryBodyPart::RightHand));
    TestTrue(TEXT("six region right arm state"),C->GetRegionStates().bRightArmInjured);
    C->ResetInjuries();
    FHitResult ExternalHit; ExternalHit.BoneName=TEXT("thigh_l");
    const EInjuryRegion ExternalRegion=C->ApplyExternalHit(ExternalHit);
    TestEqual(TEXT("external hit region"),ExternalRegion,EInjuryRegion::LeftLeg);
    TestTrue(TEXT("external hit only resolves region"),FMath::IsNearlyEqual(C->GetHealth(),100.0f));
    C->ApplyDamageToPart(ExternalRegion,12.0f);
    TestTrue(TEXT("regional damage applies exact amount"),FMath::IsNearlyEqual(C->GetHealth(),88.0f));
    C->ResetInjuries();
    ExternalHit.BoneName=TEXT("head");
    const EInjuryRegion HeadRegion=C->ApplyExternalHit(ExternalHit);
    TestEqual(TEXT("external head region"),HeadRegion,EInjuryRegion::Head);
    C->ApplyDamageToPart(HeadRegion,10.0f);
    TestTrue(TEXT("head damage applies exact amount"),FMath::IsNearlyEqual(C->GetHealth(),90.0f));
    TestFalse(TEXT("head remains without wound state"),C->IsBodyPartInjured(EInjuryBodyPart::Head));
    TestFalse(TEXT("negative regional damage rejected"),C->ApplyDamageToPart(EInjuryRegion::LeftArm,-1));
    C->MaximumHealth=200; C->ResetInjuries(); C->ApplyDamageToPart(EInjuryRegion::LeftLeg,50);
    TestEqual(TEXT("maximum health adjustable"),C->GetMaxHealth(),200.f);
    TestEqual(TEXT("health percentage"),C->GetHealthPercent(),0.75f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInjuryTimerTest,"InjuryLab.Backend.TimersAndRecovery",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInjuryTimerTest::RunTest(const FString&)
{
    FInjuryTestWorld Test; auto* C=Test.Component();
    C->ApplyBulletHit(EInjuryBodyPart::Torso);
    TestEqual(TEXT("no immediate bleed"),C->GetHealth(),100.f);
    Test.Advance(0.25f); C->ApplyBulletHit(EInjuryBodyPart::Torso);
    Test.Advance(0.26f);
    TestEqual(TEXT("repeat hit does not postpone or stack bleed"),C->GetHealth(),99.f);
    Test.Advance(0.5f); TestEqual(TEXT("second half second bleed tick"),C->GetHealth(),98.f);
    C->ConfigureBleeding(0.1f,50,10); Test.Advance(0.31f);
    TestEqual(TEXT("configured bleed floor prevents death"),C->GetHealth(),10.f);
    TestTrue(TEXT("bleeding persists until treated"),C->IsBleeding());
    C->ApplyHealthDamage(5); Test.Advance(0.21f);
    TestEqual(TEXT("below bleed floor cannot heal automatically"),C->GetHealth(),5.f);
    C->TreatTorso(20); Test.Advance(0.3f);
    TestEqual(TEXT("treatment heals and stops timer"),C->GetHealth(),25.f);
    TestFalse(TEXT("treatment clears torso injury"),C->IsRegionInjured(EInjuryRegion::Torso));
    C->ResetInjuries(); Test.Advance(2.f); TestEqual(TEXT("reset cancels bleed"),C->GetHealth(),100.f);
    C->FlashDurationSeconds=0.5f; C->TriggerFlashForTest(); TestTrue(TEXT("flash starts"),C->IsFlashed());
    TestFalse(TEXT("flash is not bullet wound"),C->IsBodyPartInjured(EInjuryBodyPart::Head));
    Test.Advance(0.6f); TestFalse(TEXT("flash expires"),C->IsFlashed());
    C->ApplyBulletHit(EInjuryBodyPart::Torso); C->ApplyHealthDamage(100);
    Test.Advance(2.f); TestEqual(TEXT("dead clamps at zero"),C->GetHealth(),0.f);
    TestFalse(TEXT("dead stops bleeding"),C->IsBleeding());
    C->ResetInjuries(); C->TriggerFlashForTest(); C->DestroyComponent();
    TestFalse(TEXT("component destruction ends active flash"),C->IsFlashed());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInjuryUITest,"InjuryLab.Integration.EditableHUD",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInjuryUITest::RunTest(const FString&)
{
    FInjuryTestWorld Test;
    auto* PC=Test.World->SpawnActor<AInjuryLabPlayerController>();
    auto* Character=Test.World->SpawnActor<AInjuryLabCharacter>(); PC->Possess(Character);
    const auto Class=LoadClass<UInjuryLabWidget>(nullptr,TEXT("/Game/InjuryLab/UI/WBP_InjuryHUD.WBP_InjuryHUD_C"));
    if (!TestNotNull(TEXT("designer HUD asset"),Class)) return false;
    auto* UI=CreateWidget<UInjuryLabWidget>(Test.World,Class);
    if (!TestNotNull(TEXT("HUD instance"),UI)) return false;
    UI->TakeWidget(); UI->SetInjuryComponent(Character->Injury);
    TestNotNull(TEXT("five experiment buttons"),UI->WidgetTree->FindWidget<UButton>(TEXT("LegButton")));
    UI->WidgetTree->FindWidget<UButton>(TEXT("HeadButton"))->OnClicked.Broadcast(); TestEqual(TEXT("head button routes stun"),Character->Injury->GetHeadState(),EInjuryHeadState::StunGrenade);
    auto* Head=UI->WidgetTree->FindWidget<UBorder>(TEXT("Part_Head"));
    TestTrue(TEXT("head indicator remains gray"),Head && Head->GetBrushColor().R<0.8f);
    TestEqual(TEXT("head action does not damage HP"),Character->Injury->GetHealth(),100.f);
    UI->WidgetTree->FindWidget<UButton>(TEXT("ResetButton"))->OnClicked.Broadcast(); UI->WidgetTree->FindWidget<UButton>(TEXT("ArmButton"))->OnClicked.Broadcast();
    TestTrue(TEXT("arm UI routes injury"),Character->Injury->IsBodyPartInjured(EInjuryBodyPart::LeftUpperArm));
    UI->RunExperiment(6);
    Character->GetCharacterMovement()->MaxWalkSpeed=420.f;
    UI->WidgetTree->FindWidget<UButton>(TEXT("LegButton"))->OnClicked.Broadcast();
    TestTrue(TEXT("leg button routes left leg"),Character->Injury->IsRegionInjured(EInjuryRegion::LeftLeg));
    TestEqual(TEXT("leg injury retains movement interface multiplier"),Character->Injury->GetMoveSpeedMultiplier(),0.6f);
    TestEqual(TEXT("leg injury does not apply movement effect"),Character->GetCharacterMovement()->MaxWalkSpeed,420.f);
    UI->RunExperiment(6); TestEqual(TEXT("reset does not overwrite external movement speed"),Character->GetCharacterMovement()->MaxWalkSpeed,420.f);
    UI->WidgetTree->FindWidget<UButton>(TEXT("FlashButton"))->OnClicked.Broadcast(); TestTrue(TEXT("flash UI routes request"),Character->Injury->IsFlashed());
    UI->RunExperiment(6); UI->WidgetTree->FindWidget<UButton>(TEXT("TorsoButton"))->OnClicked.Broadcast(); Test.Advance(1.01f);
    TestEqual(TEXT("torso UI routes timer"),Character->Injury->GetHealth(),98.f);
    UI->RunExperiment(6); UI->WidgetTree->FindWidget<UButton>(TEXT("Damage25Button"))->OnClicked.Broadcast(); TestEqual(TEXT("25 button routes tier"),Character->Injury->GetBloodVisionTier(),1);
    UI->RefreshDisplay();
    const auto* HP=UI->WidgetTree->FindWidget<UTextBlock>(TEXT("HealthText"));
    TestTrue(TEXT("health text reads new state"),HP && HP->GetText().ToString().Contains(TEXT("75")));
    auto* HealthPanel=UI->WidgetTree->FindWidget(TEXT("HealthPanel"));
    auto* HealthSlot=Cast<UCanvasPanelSlot>(HealthPanel->GetParent()->Slot);
    TestTrue(TEXT("health UI anchored top left"),HealthSlot && HealthSlot->GetAnchors().Minimum==FVector2D::ZeroVector);
    TestEqual(TEXT("old tier hidden"),UI->WidgetTree->FindWidget(TEXT("TierText"))->GetVisibility(),ESlateVisibility::Collapsed);
    TestEqual(TEXT("recovery panel hidden"),UI->WidgetTree->FindWidget(TEXT("RecoveryPanel"))->GetParent()->GetVisibility(),ESlateVisibility::Collapsed);
    PC->DebugReset(); PC->DebugFlash(); TestEqual(TEXT("key 1 entry"),Character->Injury->GetHeadState(),EInjuryHeadState::Flashbang);
    PC->DebugTorso(); PC->DebugLeftArm(); PC->DebugRightArm(); PC->DebugLeftLeg(); PC->DebugRightLeg();
    const auto Regions=Character->Injury->GetRegionStates();
    TestTrue(TEXT("keys 2 through 6 independently injure five regions"),Regions.bTorsoInjured && Regions.bLeftArmInjured && Regions.bRightArmInjured && Regions.bLeftLegInjured && Regions.bRightLegInjured);
    PC->DebugStun(); TestEqual(TEXT("key 7 entry"),Character->Injury->GetHeadState(),EInjuryHeadState::StunGrenade);
    PC->DebugReset(); TestFalse(TEXT("R entry clears injuries"),Character->Injury->GetRegionStates().bRightLegInjured);
    TestEqual(TEXT("R clears blindness"),Character->Injury->GetHeadState(),EInjuryHeadState::Normal);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInjuryBlindTest,"InjuryLab.Backend.BlindnessInterfaces",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInjuryBlindTest::RunTest(const FString&)
{
    FInjuryTestWorld Test; auto* C=Test.Component(); TStrongObjectPtr<UInjuryEventProbe> Probe(NewObject<UInjuryEventProbe>());
    C->OnBlindRequested.AddDynamic(Probe.Get(),&UInjuryEventProbe::Blind);
    C->OnBlindEnded.AddDynamic(Probe.Get(),&UInjuryEventProbe::Ended);
    C->OnDeath.AddDynamic(Probe.Get(),&UInjuryEventProbe::Died);
    C->FlashDurationSeconds=1; C->BlindInitialOpacity=0.8f; C->TriggerFlashForTest();
    TestEqual(TEXT("flash event type"),Probe->LastBlind,EInjuryHeadState::Flashbang);
    TestEqual(TEXT("flash event opacity"),Probe->Opacity,0.8f); TestEqual(TEXT("flash event duration"),Probe->Duration,1.f);
    Test.Advance(0.5f); TestTrue(TEXT("Ease Out opacity at halfway"),FMath::IsNearlyEqual(C->GetBlindOpacity(),0.2f,0.01f));
    auto* Curve=NewObject<UCurveFloat>(C); Curve->FloatCurve.AddKey(0,1); Curve->FloatCurve.AddKey(1,0); C->BlindFadeCurve=Curve;
    C->TriggerFlashForTest(); Test.Advance(0.5f);
    TestTrue(TEXT("editable curve drives alpha"),FMath::IsNearlyEqual(C->GetBlindOpacity(),0.4f,0.01f));
    C->TriggerStunForTest(); TestEqual(TEXT("stun event type"),Probe->LastBlind,EInjuryHeadState::StunGrenade);
    TestEqual(TEXT("switch emits flash end"),Probe->LastEnded,EInjuryHeadState::Flashbang);
    TestFalse(TEXT("stun does not report flashbang"),C->IsFlashed());
    TestFalse(TEXT("no head wound from either grenade"),C->IsBodyPartInjured(EInjuryBodyPart::Head));
    Test.Advance(1.6f); TestEqual(TEXT("expiry restores head state"),C->GetHeadState(),EInjuryHeadState::Normal);
    TestEqual(TEXT("expiry opacity zero"),C->GetBlindOpacity(),0.f);
    TestEqual(TEXT("expiry emits stun end"),Probe->LastEnded,EInjuryHeadState::StunGrenade);
    C->FlashDurationSeconds=2; C->TriggerFlashForTest(); Test.Advance(0.3f); C->FlashDurationSeconds=0.1f; C->TriggerFlashForTest();
    TestTrue(TEXT("short repeat does not shorten active flash"),C->GetBlindRemainingSeconds()>1.6f);
    C->ResetInjuries(); TestFalse(TEXT("reset ends blindness"),C->IsBlinded());
    C->TriggerStunForTest(); C->ApplyHealthDamage(100); TestEqual(TEXT("direct damage can still kill"),Probe->Deaths,1);
    TestFalse(TEXT("death clears blindness"),C->IsBlinded());
    C->ResetInjuries(); C->TriggerStunForTest(); C->DestroyComponent(); TestFalse(TEXT("destruction clears blindness"),C->IsBlinded());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInjuryModifierTest,"InjuryLab.Backend.RegionModifiers",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInjuryModifierTest::RunTest(const FString&)
{
    FInjuryTestWorld Test; auto* C=Test.Component(); TStrongObjectPtr<UInjuryEventProbe> Probe(NewObject<UInjuryEventProbe>());
    C->OnGameplayModifiersChanged.AddDynamic(Probe.Get(),&UInjuryEventProbe::Modifiers);
    C->OnRegionInjuryChanged.AddDynamic(Probe.Get(),&UInjuryEventProbe::Region);
    C->ApplyDamageToPart(EInjuryRegion::LeftArm,10);
    TestEqual(TEXT("explicit regional damage"),C->GetHealth(),90.f);
    TestEqual(TEXT("left arm region event"),Probe->LastRegion,EInjuryRegion::LeftArm);
    TestEqual(TEXT("duration event multiplier"),Probe->ReloadDuration,1.5f);
    TestTrue(TEXT("animation speed reciprocal"),FMath::IsNearlyEqual(Probe->ReloadSpeed,1.f/1.5f));
    C->ApplyDamageToPart(EInjuryRegion::RightArm,0); TestEqual(TEXT("default dual arms unified"),C->GetReloadDurationMultiplier(),1.5f);
    C->ArmStackingPolicy=EInjuryStackingPolicy::StackBothSides; C->ApplyDamageToPart(EInjuryRegion::RightArm,0);
    TestEqual(TEXT("optional dual arm stacking"),C->GetReloadDurationMultiplier(),2.25f);
    C->ApplyDamageToPart(EInjuryRegion::LeftLeg,0); C->ApplyDamageToPart(EInjuryRegion::RightLeg,0);
    TestEqual(TEXT("default dual legs unified"),C->GetMoveSpeedMultiplier(),0.6f);
    C->LegStackingPolicy=EInjuryStackingPolicy::StackBothSides; C->ApplyDamageToPart(EInjuryRegion::RightLeg,0);
    TestTrue(TEXT("optional dual leg stacking"),FMath::IsNearlyEqual(Probe->MoveSpeed,0.36f));
    TestEqual(TEXT("new spec has no recoil penalty"),C->GetRecoilMultiplier(),1.f);
    C->ResetInjuries(); TestEqual(TEXT("reset duration event"),Probe->ReloadDuration,1.f); TestEqual(TEXT("reset speed event"),Probe->MoveSpeed,1.f);
    return true;
}
#endif
