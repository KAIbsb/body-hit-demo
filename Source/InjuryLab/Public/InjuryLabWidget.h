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
    // 自动化测试与 UI 点击均调用同一个入口。
    UFUNCTION(BlueprintCallable, Category="Injury|Experiment") void RunExperiment(int32 Experiment);
protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
private:
    UFUNCTION() void HeadClicked();
    UFUNCTION() void FlashClicked();
    UFUNCTION() void ArmClicked();
    UFUNCTION() void TorsoClicked();
    UFUNCTION() void LegClicked();
    UFUNCTION() void DamageClicked();
    UFUNCTION() void ResetClicked();
    float RefreshClock=0;
};
