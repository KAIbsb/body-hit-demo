#include "InjuryLabPlayerController.h"
#include "InjuryLabWidget.h"
#include "InjuryComponent.h"
#include "Engine/LocalPlayer.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Border.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "TimerManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
void AInjuryLabPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (!IsLocalController() || !GetLocalPlayer()) return;
    if (!InjuryHUDClass) InjuryHUDClass=LoadClass<UInjuryLabWidget>(nullptr,TEXT("/Game/InjuryLab/UI/WBP_InjuryHUD.WBP_InjuryHUD_C"));
    if (InjuryHUDClass)
    {
        InjuryHUD=CreateWidget<UInjuryLabWidget>(this,InjuryHUDClass);
        if (InjuryHUD) { InjuryHUD->AddToViewport(); InjuryHUD->SetInjuryComponent(GetPawn() ? GetPawn()->FindComponentByClass<UInjuryComponent>() : nullptr); }
    }
    bShowMouseCursor=false; ToggleExperimentCursor();
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(),TEXT("InjuryLabSmokeTest")))
    {
        SetControlRotation(FRotator(-8,0,0));
        GetWorldTimerManager().SetTimer(SmokeTimer,this,&AInjuryLabPlayerController::RunSmokeStage,4.f,false);
    }
#endif
}
void AInjuryLabPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::Tab,IE_Pressed,this,&AInjuryLabPlayerController::ToggleExperimentCursor);
    InputComponent->BindKey(EKeys::One,IE_Pressed,this,&AInjuryLabPlayerController::DebugFlash);
    InputComponent->BindKey(EKeys::Two,IE_Pressed,this,&AInjuryLabPlayerController::DebugTorso);
    InputComponent->BindKey(EKeys::Three,IE_Pressed,this,&AInjuryLabPlayerController::DebugLeftArm);
    InputComponent->BindKey(EKeys::Four,IE_Pressed,this,&AInjuryLabPlayerController::DebugRightArm);
    InputComponent->BindKey(EKeys::Five,IE_Pressed,this,&AInjuryLabPlayerController::DebugLeftLeg);
    InputComponent->BindKey(EKeys::Six,IE_Pressed,this,&AInjuryLabPlayerController::DebugRightLeg);
    InputComponent->BindKey(EKeys::Seven,IE_Pressed,this,&AInjuryLabPlayerController::DebugStun);
    InputComponent->BindKey(EKeys::R,IE_Pressed,this,&AInjuryLabPlayerController::DebugReset);
}
UInjuryComponent* AInjuryLabPlayerController::GetInjury() const { return GetPawn() ? GetPawn()->FindComponentByClass<UInjuryComponent>() : nullptr; }
void AInjuryLabPlayerController::DebugFlash() { if (auto* C=GetInjury()) C->TriggerFlashForTest(); if (InjuryHUD) InjuryHUD->RefreshDisplay(); }
void AInjuryLabPlayerController::DebugTorso() { if (auto* C=GetInjury()) C->ApplyDamageToPart(EInjuryRegion::Torso,0); if (InjuryHUD) InjuryHUD->RefreshDisplay(); }
void AInjuryLabPlayerController::DebugLeftArm() { if (auto* C=GetInjury()) C->ApplyDamageToPart(EInjuryRegion::LeftArm,0); if (InjuryHUD) InjuryHUD->RefreshDisplay(); }
void AInjuryLabPlayerController::DebugRightArm() { if (auto* C=GetInjury()) C->ApplyDamageToPart(EInjuryRegion::RightArm,0); if (InjuryHUD) InjuryHUD->RefreshDisplay(); }
void AInjuryLabPlayerController::DebugLeftLeg() { if (auto* C=GetInjury()) C->ApplyDamageToPart(EInjuryRegion::LeftLeg,0); if (InjuryHUD) InjuryHUD->RefreshDisplay(); }
void AInjuryLabPlayerController::DebugRightLeg() { if (auto* C=GetInjury()) C->ApplyDamageToPart(EInjuryRegion::RightLeg,0); if (InjuryHUD) InjuryHUD->RefreshDisplay(); }
void AInjuryLabPlayerController::DebugStun() { if (auto* C=GetInjury()) C->TriggerStunForTest(); if (InjuryHUD) InjuryHUD->RefreshDisplay(); }
void AInjuryLabPlayerController::DebugReset() { if (auto* C=GetInjury()) C->ResetInjuries(); if (InjuryHUD) InjuryHUD->RefreshDisplay(); }
void AInjuryLabPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    if (InjuryHUD) InjuryHUD->SetInjuryComponent(InPawn ? InPawn->FindComponentByClass<UInjuryComponent>() : nullptr);
}
void AInjuryLabPlayerController::ToggleExperimentCursor()
{
    bShowMouseCursor=!bShowMouseCursor;
    if (bShowMouseCursor)
    {
        FInputModeGameAndUI Mode; Mode.SetHideCursorDuringCapture(false); Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); SetInputMode(Mode);
    }
    else SetInputMode(FInputModeGameOnly());
}
void AInjuryLabPlayerController::RunSmokeStage()
{
#if !UE_BUILD_SHIPPING
    auto Check=[this](bool OK,const TCHAR* Name) { ++SmokeChecks; if (!OK) ++SmokeFailures; UE_LOG(LogTemp,Display,TEXT("INJURY_SMOKE %s: %s"),Name,OK ? TEXT("PASS") : TEXT("FAIL")); };
    if (!InjuryHUD || !InjuryHUD->InjuryComponent) { Check(false,TEXT("HUD connected")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
    auto* C=InjuryHUD->InjuryComponent.Get();
    auto Click=[this](FName Name) { if (auto* B=InjuryHUD->WidgetTree->FindWidget<UButton>(Name)) B->OnClicked.Broadcast(); else ++SmokeFailures; };
    auto Key=[this,&Check](FKey Pressed)
    {
        bool Found=false;
        if (InputComponent) for (auto& Binding:InputComponent->KeyBindings)
            if (Binding.Chord.Key==Pressed && Binding.KeyEvent==IE_Pressed) { Binding.KeyDelegate.Execute(Pressed); Found=true; break; }
        Check(Found,*FString::Printf(TEXT("registered debug key %s"),*Pressed.GetFName().ToString()));
    };
    auto Screenshot=[](const TCHAR* Name)
    {
        const FString Directory=FPaths::ProjectSavedDir()/TEXT("Verification/Screenshots"); IFileManager::Get().MakeDirectory(*Directory,true);
        FScreenshotRequest::RequestScreenshot(Directory/Name,true,false);
    };
    switch (SmokeStage++)
    {
        case 0: Check(C->GetHealth()==100,TEXT("healthy initial state")); Screenshot(TEXT("01_Healthy.png")); break;
        case 1:
            Click(TEXT("HeadButton")); Check(C->GetHealth()==100 && C->GetHeadState()==EInjuryHeadState::StunGrenade && !C->IsRegionInjured(EInjuryRegion::Head),TEXT("head button requests stun without injury")); Screenshot(TEXT("02_Head.png")); break;
        case 2:
            Key(EKeys::R); Click(TEXT("ArmButton"));
            Check(C->GetRecoilMultiplier()==1 && C->GetReloadDurationMultiplier()==1.5f,TEXT("arm reload interface"));
            Click(TEXT("LegButton")); Key(EKeys::Six);
            Check(C->IsRegionInjured(EInjuryRegion::LeftLeg) && C->IsRegionInjured(EInjuryRegion::RightLeg) && C->GetMoveSpeedMultiplier()==0.6f && CastChecked<ACharacter>(GetPawn())->GetCharacterMovement()->MaxWalkSpeed==600.f,TEXT("leg interfaces without movement effect")); Screenshot(TEXT("03_ArmAndFoot.png")); break;
        case 3: Key(EKeys::R); Click(TEXT("TorsoButton")); Check(C->GetHealth()==100 && C->IsBleeding(),TEXT("torso starts timer")); break;
        case 4: Check(C->GetHealth()<=97 && C->GetHealth()>=95,TEXT("half second real time bleed")); Screenshot(TEXT("04_Bleeding.png")); break;
        case 5:
            Key(EKeys::R); Key(EKeys::Two); Key(EKeys::Three); Key(EKeys::Four); Key(EKeys::Five); Key(EKeys::Six);
            Check(C->GetRegionStates().bTorsoInjured && C->GetRegionStates().bLeftArmInjured && C->GetRegionStates().bRightArmInjured && C->GetRegionStates().bLeftLegInjured && C->GetRegionStates().bRightLegInjured,TEXT("five region key routing"));
            C->ConfigureBleeding(0.1f,100,1); break;
        case 6:
            Check(C->GetHealth()==1 && !C->IsDead() && C->IsBleeding(),TEXT("bleed floor keeps actor alive")); Screenshot(TEXT("05_BleedFloor.png"));
            C->TreatTorso(99); Check(C->GetHealth()==100 && !C->IsBleeding(),TEXT("treatment stops bleeding"));
            Key(EKeys::One); Check(C->IsFlashed() && !C->IsBodyPartInjured(EInjuryBodyPart::Head),TEXT("key 1 flash request")); Screenshot(TEXT("06_FlashRequest.png")); break;
        case 7:
        {
            Check(C->IsFlashed() && C->GetBlindOpacity()>0 && C->GetBlindOpacity()<0.2f,TEXT("Ease Out alpha in actual game time"));
            Key(EKeys::Seven); Check(C->GetHeadState()==EInjuryHeadState::StunGrenade,TEXT("key 7 stun request")); Key(EKeys::R);
            Click(TEXT("FlashButton")); Check(C->IsFlashed(),TEXT("flash button separate entry")); Key(EKeys::R);
            FVector Eye; FRotator Rot; GetPlayerViewPoint(Eye,Rot); const FVector Front=Eye+Rot.Vector()*300;
            Check(C->EvaluateFlashExplosion(Front,nullptr),TEXT("visible explosion accepted")); C->ResetInjuries();
            Check(!C->EvaluateFlashExplosion(Eye-Rot.Vector()*300,nullptr),TEXT("behind camera rejected"));
            Check(!C->EvaluateFlashExplosion(Eye+Rot.Vector()*3000,nullptr),TEXT("distant explosion rejected"));
            int32 ViewW=0,ViewH=0; GetViewportSize(ViewW,ViewH);
            FVector ScreenOrigin,ScreenDirection;
            DeprojectScreenPositionToWorld(ViewW*0.97f,ViewH*0.5f,ScreenOrigin,ScreenDirection);
            Check(C->EvaluateFlashExplosion(ScreenOrigin+ScreenDirection*300,nullptr),TEXT("screen edge visible accepted")); C->ResetInjuries();
            DeprojectScreenPositionToWorld(ViewW*1.08f,ViewH*0.5f,ScreenOrigin,ScreenDirection);
            Check(!C->EvaluateFlashExplosion(ScreenOrigin+ScreenDirection*300,nullptr),TEXT("outside screen rejected"));
            auto* Wall=GetWorld()->SpawnActor<AActor>(); auto* Box=NewObject<UBoxComponent>(Wall); Wall->SetRootComponent(Box);
            Box->SetBoxExtent(FVector(35)); Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Box->SetCollisionResponseToAllChannels(ECR_Ignore); Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
            Box->RegisterComponent(); Wall->SetActorLocation(Eye+Rot.Vector()*150);
            Check(!C->EvaluateFlashExplosion(Front,nullptr),TEXT("occluded explosion rejected")); Wall->Destroy();
            Check(C->GetHealth()==100 && !C->IsBleeding() && C->GetMoveSpeedMultiplier()==1,TEXT("reset restores state")); Screenshot(TEXT("07_Reset.png")); break;
        }
        default:
        {
            const FString Result=FString::Printf(TEXT("InjuryLab rendered smoke test: %s\nChecks: %d\nFailures: %d\n"),SmokeFailures==0 ? TEXT("PASS") : TEXT("FAIL"),SmokeChecks,SmokeFailures);
            FFileHelper::SaveStringToFile(Result,*(FPaths::ProjectSavedDir()/TEXT("Verification/SmokeResult.txt")));
            UE_LOG(LogTemp,Display,TEXT("INJURY_SMOKE_COMPLETE failures=%d"),SmokeFailures);
            FPlatformMisc::RequestExitWithStatus(false,SmokeFailures ? 1 : 0); return;
        }
    }
    GetWorldTimerManager().SetTimer(SmokeTimer,this,&AInjuryLabPlayerController::RunSmokeStage,2.f,false);
#endif
}
