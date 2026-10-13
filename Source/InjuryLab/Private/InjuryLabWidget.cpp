#include "InjuryLabWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/ComboBoxString.h"
void UInjuryLabWidget::NativeConstruct()
{
    Super::NativeConstruct();
    const FName Names[]={TEXT("HeadButton"),TEXT("FlashButton"),TEXT("ArmButton"),TEXT("TorsoButton"),TEXT("LegButton"),TEXT("Damage25Button"),TEXT("ResetButton")};
    for (int I=0; I<7; ++I) if (auto* B=WidgetTree->FindWidget<UButton>(Names[I]))
    {
        switch (I)
        {
            case 0: B->OnClicked.AddUniqueDynamic(this,&UInjuryLabWidget::HeadClicked); break;
            case 1: B->OnClicked.AddUniqueDynamic(this,&UInjuryLabWidget::FlashClicked); break;
            case 2: B->OnClicked.AddUniqueDynamic(this,&UInjuryLabWidget::ArmClicked); break;
            case 3: B->OnClicked.AddUniqueDynamic(this,&UInjuryLabWidget::TorsoClicked); break;
            case 4: B->OnClicked.AddUniqueDynamic(this,&UInjuryLabWidget::LegClicked); break;
            case 5: B->OnClicked.AddUniqueDynamic(this,&UInjuryLabWidget::DamageClicked); break;
            case 6: B->OnClicked.AddUniqueDynamic(this,&UInjuryLabWidget::ResetClicked); break;
        }
    }
    RefreshDisplay();
}
void UInjuryLabWidget::SetInjuryComponent(UInjuryComponent* C) { InjuryComponent=C; RefreshDisplay(); }
void UInjuryLabWidget::NativeTick(const FGeometry& G, float DT)
{
    Super::NativeTick(G,DT); RefreshClock+=DT;
    if (RefreshClock>=0.1f) { RefreshClock=0; RefreshDisplay(); }
}
void UInjuryLabWidget::HeadClicked() { RunExperiment(0); }
void UInjuryLabWidget::FlashClicked() { RunExperiment(1); }
void UInjuryLabWidget::ArmClicked() { RunExperiment(2); }
void UInjuryLabWidget::TorsoClicked() { RunExperiment(3); }
void UInjuryLabWidget::LegClicked() { RunExperiment(4); }
void UInjuryLabWidget::DamageClicked() { RunExperiment(5); }
void UInjuryLabWidget::ResetClicked() { RunExperiment(6); }
void UInjuryLabWidget::RunExperiment(int32 E)
{
    if (!InjuryComponent) return;
    switch (E)
    {
        case 0: InjuryComponent->TriggerStunForTest(); break;
        case 1: InjuryComponent->TriggerFlashForTest(); break;
        case 2:
        {
            InjuryComponent->ApplyDamageToPart(EInjuryRegion::LeftArm,0); break;
        }
        case 3: InjuryComponent->ApplyDamageToPart(EInjuryRegion::Torso,0); break;
        case 4:
        {
            InjuryComponent->ApplyDamageToPart(EInjuryRegion::LeftLeg,0); break;
        }
        case 5: InjuryComponent->TestDamage25(); break;
        case 6: InjuryComponent->ResetInjuries(); break;
        default: return;
    }
    RefreshDisplay();
}
void UInjuryLabWidget::RefreshDisplay()
{
    if (!WidgetTree || !InjuryComponent) return;
    auto Text=[this](FName Name,const FString& Value) { if (auto* T=WidgetTree->FindWidget<UTextBlock>(Name)) T->SetText(FText::FromString(Value)); };
    Text(TEXT("HealthText"),FString::Printf(TEXT("HP  %.0f / %.0f"),InjuryComponent->GetHealth(),InjuryComponent->GetMaxHealth()));
    Text(TEXT("TierText"),FString::Printf(TEXT("血丝接口档位：%d / 4"),InjuryComponent->GetBloodVisionTier()));
    Text(TEXT("ModifierText"),FString::Printf(TEXT("后坐力 ×%.2f\n换弹耗时 ×%.2f\n移速接口 ×%.2f"),InjuryComponent->GetRecoilMultiplier(),InjuryComponent->GetReloadDurationMultiplier(),InjuryComponent->GetMoveSpeedMultiplier()));
    Text(TEXT("StatusText"),InjuryComponent->IsDead() ? TEXT("血量已归零，按 R 重置") : InjuryComponent->IsBleeding() ? TEXT("躯干流血中") : TEXT("调试按键 1–7，R 重置"));
    Text(TEXT("FlashText"),InjuryComponent->IsFlashed() ? FString::Printf(TEXT("白屏接口：强度 1，剩余 %.1f 秒"),InjuryComponent->GetFlashRemainingSeconds()) : TEXT("白屏接口：待触发"));
    const auto HeadState=InjuryComponent->GetHeadState();
    Text(TEXT("HeadStateText"),HeadState==EInjuryHeadState::Flashbang ? TEXT("头部：遭受闪光弹") : HeadState==EInjuryHeadState::StunGrenade ? TEXT("头部：遭受震撼弹") : TEXT("头部：正常"));
    if (auto* P=WidgetTree->FindWidget<UProgressBar>(TEXT("HealthBar"))) P->SetPercent(InjuryComponent->GetHealthPercent());
    // MC 式六块小人：手臂和腿的细分伤口在 UI 上合并为一整块。
    auto Paint=[this](FName Name,bool bInjured)
    {
        if (auto* B=WidgetTree->FindWidget<UBorder>(Name))
            B->SetBrushColor(bInjured ? FLinearColor(0.88f,0.07f,0.07f,1) : FLinearColor(0.38f,0.40f,0.44f,1));
    };
    auto Injured=[this](EInjuryBodyPart Part) { return InjuryComponent->IsBodyPartInjured(Part); };
    Paint(TEXT("Part_Head"),false); // 头部没有受损状态，也不以红色表示致盲。
    Paint(TEXT("Part_Torso"),Injured(EInjuryBodyPart::Torso));
    Paint(TEXT("Part_LeftArm"),Injured(EInjuryBodyPart::LeftUpperArm) || Injured(EInjuryBodyPart::LeftForearm) || Injured(EInjuryBodyPart::LeftHand));
    Paint(TEXT("Part_RightArm"),Injured(EInjuryBodyPart::RightUpperArm) || Injured(EInjuryBodyPart::RightForearm) || Injured(EInjuryBodyPart::RightHand));
    Paint(TEXT("Part_LeftLeg"),Injured(EInjuryBodyPart::LeftThigh) || Injured(EInjuryBodyPart::LeftCalf) || Injured(EInjuryBodyPart::LeftFoot));
    Paint(TEXT("Part_RightLeg"),Injured(EInjuryBodyPart::RightThigh) || Injured(EInjuryBodyPart::RightCalf) || Injured(EInjuryBodyPart::RightFoot));
}
