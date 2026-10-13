#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InjuryLabCharacter.generated.h"
class UInjuryComponent;
class UCameraComponent;
UCLASS(Blueprintable)
class INJURYLAB_API AInjuryLabCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    AInjuryLabCharacter();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Injury") TObjectPtr<UInjuryComponent> Injury;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera") TObjectPtr<UCameraComponent> FirstPersonCamera;
protected:
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void LookYaw(float Value);
    void LookPitch(float Value);
};
