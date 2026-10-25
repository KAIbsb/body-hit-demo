#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "InjuryComponent.h"
#include "InjuryLabCharacter.h"
#include "InjuryLabWidget.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
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
    FInjuryTestWorld Test; auto* C=Test.Component(); C->PartMaxHealth=10.f;
    TestEqual(TEXT("initial health"),C->GetHealth(),100.f);
    TestTrue(TEXT("head region accepts damage"),C->ApplyDamageToPart(EInjuryRegion::Head,80));
    TestEqual(TEXT("head damage lowers health"),C->GetHealth(),20.f);
    TestFalse(TEXT("head has no wound"),C->IsRegionInjured(EInjuryRegion::Head));
    C->ResetInjuries();
    TestEqual(TEXT("full health tier"),C->GetBloodVisionTier(),0);
    C->ApplyHealthDamage(70); TestEqual(TEXT("30 percent enters tier one"),C->GetBloodVisionTier(),1);
    C->ApplyHealthDamage(10); TestEqual(TEXT("20 percent enters tier two"),C->GetBloodVisionTier(),2);
    C->ApplyHealthDamage(10); TestEqual(TEXT("10 percent enters tier three"),C->GetBloodVisionTier(),3);
    C->ApplyHealthDamage(5); TestEqual(TEXT("5 percent enters tier four"),C->GetBloodVisionTier(),4);
    C->ApplyHealthDamage(5); TestEqual(TEXT("zero health stays tier four"),C->GetBloodVisionTier(),4);
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
    C->ApplyDamageToPart(EInjuryRegion::Torso,21);
    TestEqual(TEXT("threshold hit applies damage once"),C->GetHealth(),79.f);
    Test.Advance(0.25f); C->ApplyDamageToPart(EInjuryRegion::Torso,0);
    Test.Advance(0.26f);
    TestEqual(TEXT("repeat hit does not postpone or stack bleed"),C->GetHealth(),78.f);
    Test.Advance(0.5f); TestEqual(TEXT("second half second bleed tick"),C->GetHealth(),77.f);
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
    C->ApplyDamageToPart(EInjuryRegion::Torso,21); C->ApplyHealthDamage(100);
    Test.Advance(2.f); TestEqual(TEXT("dead clamps at zero"),C->GetHealth(),0.f);
    TestFalse(TEXT("dead stops bleeding"),C->IsBleeding());
    C->ResetInjuries(); C->TriggerFlashForTest(); C->DestroyComponent();
    TestFalse(TEXT("component destruction ends active flash"),C->IsFlashed());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInjuryUITest,"InjuryLab.Integration.HudBinding",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInjuryUITest::RunTest(const FString&)
{
    FInjuryTestWorld Test;
    auto* Character=Test.World->SpawnActor<AInjuryLabCharacter>();
    const auto Class=LoadClass<UInjuryLabWidget>(nullptr,TEXT("/Game/InjuryLab/UI/WBP_InjuryHUD.WBP_InjuryHUD_C"));
    if (!TestNotNull(TEXT("designer HUD asset"),Class)) return false;
    auto* UI=CreateWidget<UInjuryLabWidget>(Test.World,Class);
    if (!TestNotNull(TEXT("HUD instance"),UI)) return false;
    UI->TakeWidget(); UI->SetInjuryComponent(Character->Injury);
    Character->Injury->PartMaxHealth=10.f; Character->Injury->ResetInjuries();
    auto* HP=UI->WidgetTree->FindWidget<UTextBlock>(TEXT("HealthText"));
    auto* Bar=UI->WidgetTree->FindWidget<UProgressBar>(TEXT("HealthBar"));
    auto* PartBar=UI->WidgetTree->FindWidget<UProgressBar>(TEXT("ProgressBar_Part_Torso"));
    auto* Torso=UI->WidgetTree->FindWidget<UBorder>(TEXT("Part_Torso"));
    auto* Head=UI->WidgetTree->FindWidget<UBorder>(TEXT("Part_Head"));
    if (!TestNotNull(TEXT("health text"),HP) || !TestNotNull(TEXT("health bar"),Bar) || !TestNotNull(TEXT("part bar"),PartBar) || !TestNotNull(TEXT("torso block"),Torso) || !TestNotNull(TEXT("head block"),Head)) return false;
    TestTrue(TEXT("healthy text"),HP->GetText().ToString().Contains(TEXT("100")));
    TestEqual(TEXT("healthy bar"),Bar->GetPercent(),1.f);
    TestEqual(TEXT("healthy part bar"),PartBar->GetPercent(),1.f);
    TestTrue(TEXT("healthy part color is teal"),PartBar->GetFillColorAndOpacity().G>0.5f && PartBar->GetFillColorAndOpacity().B>0.5f);
    TestTrue(TEXT("healthy border neutral"),Torso->GetBrushColor().R<0.8f);
    TestTrue(TEXT("head remains neutral"),Head->GetBrushColor().R<0.8f);
    Character->Injury->ApplyDamageToPart(EInjuryRegion::Torso,7);
    UI->RefreshDisplay();
    TestTrue(TEXT("threshold text"),HP->GetText().ToString().Contains(TEXT("93")));
    TestTrue(TEXT("threshold global bar"),FMath::IsNearlyEqual(Bar->GetPercent(),0.93f));
    TestTrue(TEXT("threshold part bar"),FMath::IsNearlyEqual(PartBar->GetPercent(),0.3f));
    TestTrue(TEXT("threshold part turns red"),PartBar->GetFillColorAndOpacity().R>0.8f);
    TestTrue(TEXT("threshold border stays neutral"),Torso->GetBrushColor().R<0.8f);
    TestEqual(TEXT("threshold bar stays visible"),PartBar->GetVisibility(),ESlateVisibility::Visible);
    Character->Injury->ApplyDamageToPart(EInjuryRegion::Torso,3);
    UI->RefreshDisplay();
    TestTrue(TEXT("damage text"),HP->GetText().ToString().Contains(TEXT("90")));
    TestTrue(TEXT("damage bar"),FMath::IsNearlyEqual(Bar->GetPercent(),0.9f));
    TestEqual(TEXT("torso part bar empties"),PartBar->GetPercent(),0.f);
    TestEqual(TEXT("empty bar hidden"),PartBar->GetVisibility(),ESlateVisibility::Hidden);
    TestTrue(TEXT("empty part border turns red"),Torso->GetBrushColor().R>0.8f);
    Character->Injury->ResetInjuries(); UI->RefreshDisplay();
    TestTrue(TEXT("reset text"),HP->GetText().ToString().Contains(TEXT("100")));
    TestTrue(TEXT("reset bar"),FMath::IsNearlyEqual(Bar->GetPercent(),1.f));
    TestEqual(TEXT("reset part bar"),PartBar->GetPercent(),1.f);
    TestEqual(TEXT("reset bar visible"),PartBar->GetVisibility(),ESlateVisibility::Visible);
    TestTrue(TEXT("reset part color"),PartBar->GetFillColorAndOpacity().G>0.5f && PartBar->GetFillColorAndOpacity().B>0.5f);
    TestTrue(TEXT("reset border neutral"),Torso->GetBrushColor().R<0.8f);
    auto* HealthBorder=UI->WidgetTree->FindWidget(TEXT("Border_2"));
    auto* HealthSlot=HealthBorder ? Cast<UCanvasPanelSlot>(HealthBorder->Slot) : nullptr;
    TestTrue(TEXT("health UI anchored bottom left"),HealthSlot && HealthSlot->GetAnchors().Minimum==FVector2D(0,1));
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
    C->StunDurationSeconds=2; C->TriggerStunForTest(); TestEqual(TEXT("stun event type"),Probe->LastBlind,EInjuryHeadState::StunGrenade);
    TestTrue(TEXT("flash survives stun"),C->IsFlashed() && C->IsStunned());
    TestTrue(TEXT("both channels have opacity"),C->GetFlashOpacity()>0.f && C->GetStunOpacity()>0.f);
    TestTrue(TEXT("combined opacity is the stronger channel"),FMath::IsNearlyEqual(C->GetBlindOpacity(),FMath::Max(C->GetFlashOpacity(),C->GetStunOpacity()),0.001f));
    TestFalse(TEXT("no head wound from either grenade"),C->IsBodyPartInjured(EInjuryBodyPart::Head));
    Test.Advance(0.6f);
    TestFalse(TEXT("flash expires first"),C->IsFlashed());
    TestTrue(TEXT("stun outlives flash"),C->IsStunned() && C->GetStunOpacity()>0.f);
    Test.Advance(1.5f);
    TestFalse(TEXT("stun expires later"),C->IsBlinded());
    TestEqual(TEXT("expiry restores head state"),C->GetHeadState(),EInjuryHeadState::Normal);
    TestEqual(TEXT("expiry opacity zero"),C->GetBlindOpacity(),0.f);
    TestEqual(TEXT("expiry emits stun end"),Probe->LastEnded,EInjuryHeadState::StunGrenade);
    C->FlashDurationSeconds=2; C->TriggerFlashForTest(); Test.Advance(0.3f); C->FlashDurationSeconds=0.1f; C->TriggerFlashForTest();
    TestTrue(TEXT("same type re-trigger covers the previous flash"),C->GetFlashRemainingSeconds()<0.2f);
    TestTrue(TEXT("re-trigger restores full opacity"),FMath::IsNearlyEqual(C->GetFlashOpacity(),0.8f,0.01f));
    C->ResetInjuries(); TestFalse(TEXT("reset ends blindness"),C->IsBlinded());
    C->TriggerStunForTest(); C->ApplyHealthDamage(100); TestEqual(TEXT("direct damage can still kill"),Probe->Deaths,1);
    TestFalse(TEXT("death clears blindness"),C->IsBlinded());
    C->ResetInjuries(); C->TriggerStunForTest(); C->DestroyComponent(); TestFalse(TEXT("destruction clears blindness"),C->IsBlinded());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInjuryModifierTest,"InjuryLab.Backend.RegionModifiers",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInjuryModifierTest::RunTest(const FString&)
{
    FInjuryTestWorld Test; auto* C=Test.Component(); C->PartMaxHealth=10.f; TStrongObjectPtr<UInjuryEventProbe> Probe(NewObject<UInjuryEventProbe>());
    C->OnGameplayModifiersChanged.AddDynamic(Probe.Get(),&UInjuryEventProbe::Modifiers);
    C->OnRegionInjuryChanged.AddDynamic(Probe.Get(),&UInjuryEventProbe::Region);
    TestEqual(TEXT("healthy recoil"),C->GetRecoilMultiplier(),1.f);
    C->ApplyDamageToPart(EInjuryRegion::LeftArm,10);
    TestEqual(TEXT("explicit regional damage"),C->GetHealth(),90.f);
    TestEqual(TEXT("left arm region event"),Probe->LastRegion,EInjuryRegion::LeftArm);
    TestEqual(TEXT("duration event multiplier"),Probe->ReloadDuration,1.5f);
    TestTrue(TEXT("animation speed reciprocal"),FMath::IsNearlyEqual(Probe->ReloadSpeed,1.f/1.5f));
    TestEqual(TEXT("left arm does not raise recoil"),C->GetRecoilMultiplier(),1.f);
    C->ApplyDamageToPart(EInjuryRegion::RightArm,10); TestEqual(TEXT("default dual arms unified"),C->GetReloadDurationMultiplier(),1.5f);
    TestEqual(TEXT("right arm raises recoil"),C->GetRecoilMultiplier(),5.f);
    C->ArmStackingPolicy=EInjuryStackingPolicy::StackBothSides; C->ApplyDamageToPart(EInjuryRegion::RightArm,0);
    TestEqual(TEXT("optional dual arm stacking"),C->GetReloadDurationMultiplier(),2.25f);
    C->ApplyDamageToPart(EInjuryRegion::LeftLeg,10); C->ApplyDamageToPart(EInjuryRegion::RightLeg,10);
    TestEqual(TEXT("default dual legs unified"),C->GetMoveSpeedMultiplier(),0.6f);
    C->LegStackingPolicy=EInjuryStackingPolicy::StackBothSides; C->ApplyDamageToPart(EInjuryRegion::RightLeg,0);
    TestTrue(TEXT("optional dual leg stacking"),FMath::IsNearlyEqual(Probe->MoveSpeed,0.36f));
    TestEqual(TEXT("torso healthy sensitivity"),C->GetSensitivityMultiplier(),1.f);
    C->ApplyDamageToPart(EInjuryRegion::Torso,10);
    TestEqual(TEXT("torso break lowers sensitivity"),C->GetSensitivityMultiplier(),0.3f);
    C->ResetInjuries(); TestEqual(TEXT("reset duration event"),Probe->ReloadDuration,1.f); TestEqual(TEXT("reset speed event"),Probe->MoveSpeed,1.f);
    TestEqual(TEXT("reset recoil"),C->GetRecoilMultiplier(),1.f); TestEqual(TEXT("reset sensitivity"),C->GetSensitivityMultiplier(),1.f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInjuryBrokenTest,"InjuryLab.Backend.PartBrokenEvents",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInjuryBrokenTest::RunTest(const FString&)
{
    FInjuryTestWorld Test; auto* C=Test.Component(); C->PartMaxHealth=10.f; TStrongObjectPtr<UInjuryEventProbe> Probe(NewObject<UInjuryEventProbe>());
    C->OnRegionBrokenChanged.AddDynamic(Probe.Get(),&UInjuryEventProbe::Broken);
    TestFalse(TEXT("head never breaks"),C->IsRegionBroken(EInjuryRegion::Head));
    C->ApplyDamageToPart(EInjuryRegion::LeftArm,4);
    TestFalse(TEXT("partial durability stays intact"),C->IsRegionBroken(EInjuryRegion::LeftArm));
    TestEqual(TEXT("no event before zero"),Probe->BrokenEvents,0);
    C->ApplyDamageToPart(EInjuryRegion::LeftArm,6);
    TestTrue(TEXT("zero durability breaks arm"),C->IsRegionBroken(EInjuryRegion::LeftArm));
    TestEqual(TEXT("break event count"),Probe->BrokenEvents,1);
    TestEqual(TEXT("break event region"),Probe->LastBrokenRegion,EInjuryRegion::LeftArm);
    TestTrue(TEXT("break event state"),Probe->bLastBroken);
    C->ApplyDamageToPart(EInjuryRegion::LeftArm,1);
    TestEqual(TEXT("already broken does not repeat"),Probe->BrokenEvents,1);
    C->ResetInjuries();
    TestFalse(TEXT("reset repairs arm"),C->IsRegionBroken(EInjuryRegion::LeftArm));
    TestFalse(TEXT("reset broadcasts repaired state"),Probe->bLastBroken);
    TestEqual(TEXT("reset sync ends on right leg"),Probe->LastBrokenRegion,EInjuryRegion::RightLeg);
    C->ApplyDamageToPart(EInjuryRegion::Torso,10);
    TestTrue(TEXT("torso breaks"),C->IsRegionBroken(EInjuryRegion::Torso));
    C->TreatTorso(0);
    TestFalse(TEXT("treatment repairs torso durability"),C->IsRegionBroken(EInjuryRegion::Torso));
    TestFalse(TEXT("repair event state"),Probe->bLastBroken);
    TestEqual(TEXT("repair event region"),Probe->LastBrokenRegion,EInjuryRegion::Torso);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInjuryDeadPartTest,"InjuryLab.Backend.DeadPartDamage",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInjuryDeadPartTest::RunTest(const FString&)
{
    FInjuryTestWorld Test; auto* C=Test.Component(); C->PartMaxHealth=10.f; TStrongObjectPtr<UInjuryEventProbe> Probe(NewObject<UInjuryEventProbe>());
    C->OnRegionBrokenChanged.AddDynamic(Probe.Get(),&UInjuryEventProbe::Broken);
    C->ApplyHealthDamage(100);
    TestTrue(TEXT("dead state reached"),C->IsDead());
    TestTrue(TEXT("dead still accepts part damage"),C->ApplyDamageToPart(EInjuryRegion::RightArm,4));
    TestEqual(TEXT("dead part durability decreases"),C->GetPartHealth(EInjuryRegion::RightArm),6.f);
    TestFalse(TEXT("part not broken yet"),C->IsRegionBroken(EInjuryRegion::RightArm));
    C->ApplyDamageToPart(EInjuryRegion::RightArm,6);
    TestTrue(TEXT("dead part can break"),C->IsRegionBroken(EInjuryRegion::RightArm));
    TestEqual(TEXT("dead break event count"),Probe->BrokenEvents,1);
    TestEqual(TEXT("dead break event region"),Probe->LastBrokenRegion,EInjuryRegion::RightArm);
    C->ApplyDamageToPart(EInjuryRegion::Torso,10);
    TestTrue(TEXT("dead torso part breaks"),C->IsRegionBroken(EInjuryRegion::Torso));
    TestFalse(TEXT("dead torso does not restart bleeding"),C->IsBleeding());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInjuryBloodVisionTest,"InjuryLab.Backend.BloodVisionOpacity",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInjuryBloodVisionTest::RunTest(const FString&)
{
    FInjuryTestWorld Test; auto* C=Test.Component();
    C->BloodVisionMaxOpacity=0.1f; C->BloodVisionFadeSeconds=0.5f;
    TestEqual(TEXT("blood opacity starts clear"),C->GetBloodVisionOpacity(),0.f);
    C->ApplyHealthDamage(70);
    TestEqual(TEXT("first blood tier"),C->GetBloodVisionTier(),1);
    TestTrue(TEXT("opacity does not jump at once"),C->GetBloodVisionOpacity()<0.001f);
    Test.Advance(0.25f);
    const float Mid=C->GetBloodVisionOpacity();
    TestTrue(TEXT("opacity interpolates in"),Mid>0.f && Mid<0.025f);
    Test.Advance(0.35f);
    TestTrue(TEXT("tier one target"),FMath::IsNearlyEqual(C->GetBloodVisionOpacity(),0.025f,0.0005f));
    TestTrue(TEXT("custom max scales by percent"),FMath::IsNearlyEqual(C->GetBloodVisionOpacity(0.2f),0.05f,0.0005f));
    C->ApplyHealthDamage(10); Test.Advance(0.6f);
    TestTrue(TEXT("tier two target"),FMath::IsNearlyEqual(C->GetBloodVisionOpacity(),0.05f,0.0005f));
    C->ApplyHealthDamage(10); Test.Advance(0.6f);
    TestTrue(TEXT("tier three target"),FMath::IsNearlyEqual(C->GetBloodVisionOpacity(),0.075f,0.0005f));
    C->ApplyHealthDamage(5); Test.Advance(0.6f);
    TestTrue(TEXT("tier four target"),FMath::IsNearlyEqual(C->GetBloodVisionOpacity(),0.1f,0.0005f));
    C->ApplyHealthDamage(5); Test.Advance(0.6f);
    TestTrue(TEXT("zero health stays max"),FMath::IsNearlyEqual(C->GetBloodVisionOpacity(),0.1f,0.0005f));
    C->ResetInjuries(); Test.Advance(0.6f);
    TestTrue(TEXT("reset fades out"),FMath::IsNearlyEqual(C->GetBloodVisionOpacity(),0.f,0.0005f));
    return true;
}
#endif