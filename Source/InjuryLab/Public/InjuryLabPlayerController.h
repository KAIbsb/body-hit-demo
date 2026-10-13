#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InjuryLabPlayerController.generated.h"
class UInjuryLabWidget;
class UInjuryComponent;
UCLASS(Blueprintable)
class INJURYLAB_API AInjuryLabPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Injury|UI") TSubclassOf<UInjuryLabWidget> InjuryHUDClass;
    UPROPERTY(BlueprintReadOnly, Category="Injury|UI") TObjectPtr<UInjuryLabWidget> InjuryHUD;
    UFUNCTION(BlueprintCallable, Category="Injury|UI") void ToggleExperimentCursor();
    UFUNCTION(BlueprintCallable, Category="Injury|Debug") void DebugFlash();
    UFUNCTION(BlueprintCallable, Category="Injury|Debug") void DebugTorso();
    UFUNCTION(BlueprintCallable, Category="Injury|Debug") void DebugLeftArm();
    UFUNCTION(BlueprintCallable, Category="Injury|Debug") void DebugRightArm();
    UFUNCTION(BlueprintCallable, Category="Injury|Debug") void DebugLeftLeg();
    UFUNCTION(BlueprintCallable, Category="Injury|Debug") void DebugRightLeg();
    UFUNCTION(BlueprintCallable, Category="Injury|Debug") void DebugStun();
    UFUNCTION(BlueprintCallable, Category="Injury|Debug") void DebugReset();
protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void OnPossess(APawn* InPawn) override;
private:
    FTimerHandle SmokeTimer;
    int32 SmokeStage = 0;
    int32 SmokeFailures = 0;
    int32 SmokeChecks = 0;
    void RunSmokeStage();
    UInjuryComponent* GetInjury() const;
};
