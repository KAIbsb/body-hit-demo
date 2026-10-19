#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InjuryComponent.h"
#include "InjuryLabWidget.generated.h"
UCLASS(Blueprintable)
class INJURYLAB_API UInjuryLabWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, Category="Injury") TObjectPtr<UInjuryComponent> InjuryComponent;
    UFUNCTION(BlueprintCallable, Category="Injury") void SetInjuryComponent(UInjuryComponent* Component);
    UFUNCTION(BlueprintCallable, Category="Injury") void RefreshDisplay();
protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
private:
    float RefreshClock=0;
};
