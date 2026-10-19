#include "InjuryLabPlayerController.h"
#include "InjuryLabWidget.h"
#include "InjuryComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
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
}
void AInjuryLabPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    if (InjuryHUD) InjuryHUD->SetInjuryComponent(InPawn ? InPawn->FindComponentByClass<UInjuryComponent>() : nullptr);
}
