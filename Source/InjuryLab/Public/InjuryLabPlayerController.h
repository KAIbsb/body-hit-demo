#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InjuryLabPlayerController.generated.h"
class UInjuryLabWidget;

UCLASS(Blueprintable)
class INJURYLAB_API AInjuryLabPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Injury|UI") TSubclassOf<UInjuryLabWidget> InjuryHUDClass;
    UPROPERTY(BlueprintReadOnly, Category="Injury|UI") TObjectPtr<UInjuryLabWidget> InjuryHUD;
protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
};
