#include "InjuryLabWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
void UInjuryLabWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshDisplay();
}
void UInjuryLabWidget::SetInjuryComponent(UInjuryComponent* C) { InjuryComponent=C; RefreshDisplay(); }
void UInjuryLabWidget::NativeTick(const FGeometry& G, float DT)
{
    Super::NativeTick(G,DT); RefreshClock+=DT;
    if (RefreshClock>=0.1f) { RefreshClock=0; RefreshDisplay(); }
}
void UInjuryLabWidget::RefreshDisplay()
{
    if (!WidgetTree || !InjuryComponent) return;
    auto Text=[this](FName Name,const FString& Value) { if (auto* T=WidgetTree->FindWidget<UTextBlock>(Name)) T->SetText(FText::FromString(Value)); };
    Text(TEXT("HealthText"),FString::Printf(TEXT("HP  %.0f / %.0f"),InjuryComponent->GetHealth(),InjuryComponent->GetMaxHealth()));
    Text(TEXT("TierText"),FString::Printf(TEXT("血丝接口档位：%d / 4"),InjuryComponent->GetBloodVisionTier()));
    Text(TEXT("ModifierText"),FString::Printf(TEXT("后坐力 ×%.2f\n换弹耗时 ×%.2f\n移速接口 ×%.2f"),InjuryComponent->GetRecoilMultiplier(),InjuryComponent->GetReloadDurationMultiplier(),InjuryComponent->GetMoveSpeedMultiplier()));
    Text(TEXT("StatusText"),InjuryComponent->IsDead() ? TEXT("血量已归零") : InjuryComponent->IsBleeding() ? TEXT("躯干流血中") : TEXT("等待受击"));
    Text(TEXT("FlashText"),InjuryComponent->IsFlashed() ? FString::Printf(TEXT("白屏接口：强度 1，剩余 %.1f 秒"),InjuryComponent->GetFlashRemainingSeconds()) : TEXT("白屏接口：待触发"));
    const auto HeadState=InjuryComponent->GetHeadState();
    Text(TEXT("HeadStateText"),HeadState==EInjuryHeadState::Flashbang ? TEXT("头部：遭受闪光弹") : HeadState==EInjuryHeadState::StunGrenade ? TEXT("头部：遭受震撼弹") : TEXT("头部：正常"));
    if (auto* P=WidgetTree->FindWidget<UProgressBar>(TEXT("HealthBar"))) P->SetPercent(InjuryComponent->GetHealthPercent());
    const FLinearColor BarNormal=FLinearColor::FromSRGBColor(FColor(0x90,0xED,0xCD,0xFF));
    const FLinearColor BorderNormal(0.38f,0.40f,0.44f,1), Injured(0.88f,0.07f,0.07f,1);
    // 边框只在部位耐久耗尽时变红，表示该部位已损坏。
    auto Border=[this,BorderNormal,Injured](FName Name,EInjuryRegion Region)
    {
        if (auto* B=WidgetTree->FindWidget<UBorder>(Name))
            B->SetBrushColor(InjuryComponent->GetPartHealth(Region)<=0.f ? Injured : BorderNormal);
    };
    if (auto* B=WidgetTree->FindWidget<UBorder>(TEXT("Part_Head"))) B->SetBrushColor(BorderNormal);
    Border(TEXT("Part_Torso"),EInjuryRegion::Torso);
    Border(TEXT("Part_LeftArm"),EInjuryRegion::LeftArm);
    Border(TEXT("Part_RightArm"),EInjuryRegion::RightArm);
    Border(TEXT("Part_LeftLeg"),EInjuryRegion::LeftLeg);
    Border(TEXT("Part_RightLeg"),EInjuryRegion::RightLeg);
    auto Bar=[this,BarNormal,Injured](FName Name,EInjuryRegion Region)
    {
        if (auto* P=WidgetTree->FindWidget<UProgressBar>(Name))
        {
            const bool bBroken=InjuryComponent->GetPartHealth(Region)<=0.f;
            // 耐久耗尽时隐藏进度条，让下面的 Border 背景整块变红；Hidden 保留布局尺寸。
            P->SetVisibility(bBroken ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
            P->SetPercent(bBroken ? 0.f : InjuryComponent->GetPartHealthPercent(Region));
            P->SetFillColorAndOpacity(InjuryComponent->IsRegionInjured(Region) ? Injured : BarNormal);
        }
    };
    // 头部没有部位耐久，状态由 HeadStateText 表达。
    Bar(TEXT("ProgressBar_Part_Torso"),EInjuryRegion::Torso);
    Bar(TEXT("ProgressBar_Part_LeftArm"),EInjuryRegion::LeftArm);
    Bar(TEXT("ProgressBar_Part_RightArm"),EInjuryRegion::RightArm);
    Bar(TEXT("ProgressBar_Part_LeftLeg"),EInjuryRegion::LeftLeg);
    Bar(TEXT("ProgressBar_Part_RightLeg"),EInjuryRegion::RightLeg);
}
