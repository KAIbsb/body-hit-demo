#include "InjuryComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "SceneView.h"
#include "Curves/CurveFloat.h"
static_assert(static_cast<int>(EInjuryBodyPart::RightFoot)+1==static_cast<int>(InjuryRules::Part::Count));

UInjuryComponent::UInjuryComponent() { PrimaryComponentTick.bCanEverTick = false; }
void UInjuryComponent::BeginPlay()
{
    Super::BeginPlay(); SyncTuning(); State.Reset(); ResetPartHealth();
    GetOwner()->OnTakePointDamage.AddDynamic(this, &UInjuryComponent::HandlePointDamage);
}
void UInjuryComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    EndFlash();
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(BleedTimer);
    if (GetOwner()) GetOwner()->OnTakePointDamage.RemoveDynamic(this, &UInjuryComponent::HandlePointDamage);
    Super::EndPlay(Reason);
}
void UInjuryComponent::SyncTuning()
{
    auto Safe = [](float Value, float Fallback) { return FMath::IsFinite(Value) ? FMath::Max(0.f, Value) : Fallback; };
    State.MaxHealth=FMath::Max(1.f,Safe(MaximumHealth,100)); State.Health=FMath::Min(State.Health,State.MaxHealth);
    State.BleedPerTick=Safe(BleedDamagePerTick,1);
    State.BleedMinimumHealth=FMath::Clamp(Safe(BleedMinimumHealth,1),1.f,State.MaxHealth);
    State.ArmReloadDuration=FMath::Max(1.f,Safe(InjuredReloadDurationMultiplier,1.5f));
    State.LegMoveSpeed=FMath::Clamp(Safe(InjuredMoveSpeedMultiplier,0.6f),0.f,1.f);
    State.StackArmSides=ArmStackingPolicy==EInjuryStackingPolicy::StackBothSides;
    State.StackLegSides=LegStackingPolicy==EInjuryStackingPolicy::StackBothSides;
    PartMaxHealth=FMath::Max(1.f,Safe(PartMaxHealth,30));
    InjuredThreshold=FMath::Clamp(Safe(InjuredThreshold,0.3f),0.f,1.f);
    for (int I=1;I<=6;++I) PartHealth[I]=FMath::Clamp(PartHealth[I],0.f,PartMaxHealth);
}
bool UInjuryComponent::IsBodyPartInjured(EInjuryBodyPart Part) const { return State.IsInjured(static_cast<InjuryRules::Part>(Part)); }
EInjuryRegion UInjuryComponent::ToRegion(EInjuryBodyPart Part)
{
    switch (Part)
    {
        case EInjuryBodyPart::Head: return EInjuryRegion::Head;
        case EInjuryBodyPart::Torso: return EInjuryRegion::Torso;
        case EInjuryBodyPart::LeftUpperArm: case EInjuryBodyPart::LeftForearm: case EInjuryBodyPart::LeftHand: return EInjuryRegion::LeftArm;
        case EInjuryBodyPart::RightUpperArm: case EInjuryBodyPart::RightForearm: case EInjuryBodyPart::RightHand: return EInjuryRegion::RightArm;
        case EInjuryBodyPart::LeftThigh: case EInjuryBodyPart::LeftCalf: case EInjuryBodyPart::LeftFoot: return EInjuryRegion::LeftLeg;
        case EInjuryBodyPart::RightThigh: case EInjuryBodyPart::RightCalf: case EInjuryBodyPart::RightFoot: return EInjuryRegion::RightLeg;
        default: return EInjuryRegion::Unknown;
    }
}
bool UInjuryComponent::HasPartHealth(EInjuryRegion Region)
{
    const int32 I=static_cast<int32>(Region);
    return I>=static_cast<int32>(EInjuryRegion::Torso) && I<=static_cast<int32>(EInjuryRegion::RightLeg);
}
int32 UInjuryComponent::PartIndex(EInjuryRegion Region) { return static_cast<int32>(Region); }
bool UInjuryComponent::IsPartInjured(EInjuryRegion Region) const
{
    // 加一个小容差，避免 9/30 这类浮点边界判不进去。
    return HasPartHealth(Region) && GetPartHealth(Region)<=PartMaxHealth*InjuredThreshold+KINDA_SMALL_NUMBER;
}
void UInjuryComponent::DamagePart(EInjuryRegion Region, float Amount)
{
    if (!HasPartHealth(Region)) return;
    const int32 I=PartIndex(Region);
    PartHealth[I]=FMath::Clamp(PartHealth[I]-Amount,0.f,PartMaxHealth);
}
void UInjuryComponent::ResetPartHealth()
{
    for (int I=1;I<=6;++I) PartHealth[I]=PartMaxHealth;
}
float UInjuryComponent::GetPartHealth(EInjuryRegion Region) const
{
    return HasPartHealth(Region) ? PartHealth[PartIndex(Region)] : 0.f;
}
float UInjuryComponent::GetPartHealthPercent(EInjuryRegion Region) const
{
    return HasPartHealth(Region) && PartMaxHealth>0 ? FMath::Clamp(GetPartHealth(Region)/PartMaxHealth,0.f,1.f) : 0.f;
}
float UInjuryComponent::GetPartMaxHealth(EInjuryRegion Region) const { return HasPartHealth(Region) ? PartMaxHealth : 0.f; }
EInjuryRegion UInjuryComponent::ResolveRegion(FName BoneName) const { return ToRegion(ResolveBodyPart(BoneName)); }
bool UInjuryComponent::IsRegionInjured(EInjuryRegion Region) const
{
    switch (Region)
    {
        case EInjuryRegion::Torso: return State.IsInjured(InjuryRules::Part::Torso);
        case EInjuryRegion::LeftArm: return State.LeftArm();
        case EInjuryRegion::RightArm: return State.RightArm();
        case EInjuryRegion::LeftLeg: return State.LeftLeg();
        case EInjuryRegion::RightLeg: return State.RightLeg();
        default: return false;
    }
}
FInjuryRegionStates UInjuryComponent::GetRegionStates() const
{
    FInjuryRegionStates Result; Result.Head=HeadState;
    Result.bTorsoInjured=IsRegionInjured(EInjuryRegion::Torso); Result.bLeftArmInjured=State.LeftArm(); Result.bRightArmInjured=State.RightArm();
    Result.bLeftLegInjured=State.LeftLeg(); Result.bRightLegInjured=State.RightLeg(); return Result;
}
bool UInjuryComponent::ApplyDamageToPart(EInjuryRegion Part, float DamageAmount)
{
    if (Part==EInjuryRegion::Head)
    {
        if (IsDead() || !FMath::IsFinite(DamageAmount) || DamageAmount<0) return false;
        const auto Old=State; SyncTuning(); State.Damage(DamageAmount); PublishChanges(Old); return true;
    }
    EInjuryBodyPart Detail=EInjuryBodyPart::Unknown;
    switch (Part)
    {
        case EInjuryRegion::Torso: Detail=EInjuryBodyPart::Torso; break;
        case EInjuryRegion::LeftArm: Detail=EInjuryBodyPart::LeftUpperArm; break;
        case EInjuryRegion::RightArm: Detail=EInjuryBodyPart::RightUpperArm; break;
        case EInjuryRegion::LeftLeg: Detail=EInjuryBodyPart::LeftThigh; break;
        case EInjuryRegion::RightLeg: Detail=EInjuryBodyPart::RightThigh; break;
        default: return false; // 未知部位不接受伤害。
    }
    return ApplyPartDamage(Detail,DamageAmount);
}
EInjuryRegion UInjuryComponent::ApplyExternalHit(const FHitResult& Hit)
{
    return ResolveRegion(Hit.BoneName);
}

bool UInjuryComponent::ApplyPartDamage(EInjuryBodyPart Part, float Amount)
{
    const auto P=static_cast<InjuryRules::Part>(Part);
    if (!InjuryRules::State::Valid(P) || P==InjuryRules::Part::Head || IsDead() || !FMath::IsFinite(Amount) || Amount<0) return false;
    const EInjuryRegion Region=ToRegion(Part);
    if (!HasPartHealth(Region)) return false;
    const auto Old=State; SyncTuning();
    // 先扣部位耐久；只有跌破阈值才建立伤口并触发流血/倍率。
    DamagePart(Region,Amount);
    if (IsPartInjured(Region))
    {
        State.Wounds[static_cast<int>(P)]=true;
        if (P==InjuryRules::Part::Torso) State.Bleeding=true;
    }
    State.Damage(Amount);
    if (State.Bleeding && !Old.Bleeding && GetWorld())
    {
        ActiveBleedInterval=FMath::IsFinite(BleedIntervalSeconds) ? FMath::Max(0.01f,BleedIntervalSeconds) : 0.5f;
        GetWorld()->GetTimerManager().SetTimer(BleedTimer,this,&UInjuryComponent::HandleBleedTick,ActiveBleedInterval,true);
    }
    PublishChanges(Old); return true;
}
void UInjuryComponent::HandlePointDamage(AActor*, float Damage, AController*, FVector, UPrimitiveComponent*, FName BoneName, FVector, const UDamageType*, AActor*)
{
    if (FMath::IsFinite(Damage) && Damage>0) ApplyPartDamage(ResolveBodyPart(BoneName),Damage);
}
EInjuryBodyPart UInjuryComponent::ResolveBodyPart(FName BoneName) const
{
    if (const auto* Override=BoneOverrides.Find(BoneName)) return *Override;
    const FString N=BoneName.ToString().ToLower();
    const bool L=N.EndsWith(TEXT("_l")), R=N.EndsWith(TEXT("_r"));
    if (N==TEXT("head") || N.StartsWith(TEXT("neck"))) return EInjuryBodyPart::Head;
    if (N.StartsWith(TEXT("spine")) || N==TEXT("pelvis")) return EInjuryBodyPart::Torso;
    if (!L && !R) return EInjuryBodyPart::Unknown;
    if (N.StartsWith(TEXT("upperarm")) || N.StartsWith(TEXT("clavicle"))) return L ? EInjuryBodyPart::LeftUpperArm : EInjuryBodyPart::RightUpperArm;
    if (N.StartsWith(TEXT("lowerarm"))) return L ? EInjuryBodyPart::LeftForearm : EInjuryBodyPart::RightForearm;
    if (N.StartsWith(TEXT("hand")) || N.StartsWith(TEXT("thumb")) || N.StartsWith(TEXT("index")) || N.StartsWith(TEXT("middle")) || N.StartsWith(TEXT("ring")) || N.StartsWith(TEXT("pinky"))) return L ? EInjuryBodyPart::LeftHand : EInjuryBodyPart::RightHand;
    if (N.StartsWith(TEXT("thigh"))) return L ? EInjuryBodyPart::LeftThigh : EInjuryBodyPart::RightThigh;
    if (N.StartsWith(TEXT("calf"))) return L ? EInjuryBodyPart::LeftCalf : EInjuryBodyPart::RightCalf;
    if (N.StartsWith(TEXT("foot")) || N.StartsWith(TEXT("ball"))) return L ? EInjuryBodyPart::LeftFoot : EInjuryBodyPart::RightFoot;
    return EInjuryBodyPart::Unknown;
}
void UInjuryComponent::ApplyHealthDamage(float Amount) { const auto Old=State; SyncTuning(); State.Damage(Amount); PublishChanges(Old); }
void UInjuryComponent::HandleBleedTick()
{
    const auto Old=State; SyncTuning(); State.BleedTick(); PublishChanges(Old);
    const float Interval=FMath::IsFinite(BleedIntervalSeconds) ? FMath::Max(0.01f,BleedIntervalSeconds) : 0.5f;
    if (State.Bleeding && GetWorld() && !FMath::IsNearlyEqual(ActiveBleedInterval,Interval))
    {
        ActiveBleedInterval=Interval;
        GetWorld()->GetTimerManager().SetTimer(BleedTimer,this,&UInjuryComponent::HandleBleedTick,Interval,true);
    }
}
void UInjuryComponent::StopBleeding() { const auto Old=State; State.Bleeding=false; PublishChanges(Old); }
void UInjuryComponent::ConfigureBleeding(float IntervalSeconds, float DamagePerTick, float MinimumHealth)
{
    if (!FMath::IsFinite(IntervalSeconds) || !FMath::IsFinite(DamagePerTick) || !FMath::IsFinite(MinimumHealth)) return;
    BleedIntervalSeconds=FMath::Max(0.01f,IntervalSeconds); BleedDamagePerTick=FMath::Max(0.f,DamagePerTick); BleedMinimumHealth=FMath::Max(1.f,MinimumHealth);
    const auto Old=State; SyncTuning(); PublishChanges(Old);
    if (State.Bleeding && GetWorld() && !FMath::IsNearlyEqual(ActiveBleedInterval,BleedIntervalSeconds))
    {
        ActiveBleedInterval=BleedIntervalSeconds;
        GetWorld()->GetTimerManager().SetTimer(BleedTimer,this,&UInjuryComponent::HandleBleedTick,ActiveBleedInterval,true);
    }
}
void UInjuryComponent::TreatTorso(float HealAmount)
{
    if (IsDead() || !FMath::IsFinite(HealAmount) || HealAmount<0) return;
    const auto Old=State; SyncTuning(); State.Bleeding=false; State.Wounds[static_cast<int>(InjuryRules::Part::Torso)]=false;
    PartHealth[PartIndex(EInjuryRegion::Torso)]=PartMaxHealth;
    State.Health=FMath::Min(State.MaxHealth,State.Health+HealAmount); PublishChanges(Old);
}
void UInjuryComponent::PublishChanges(const InjuryRules::State& Old)
{
    if (!State.Bleeding && GetWorld()) GetWorld()->GetTimerManager().ClearTimer(BleedTimer);
    if (Old.Health!=State.Health || Old.MaxHealth!=State.MaxHealth) OnHealthChanged.Broadcast(State.Health,State.MaxHealth);
    if (Old.Tier()!=State.Tier()) OnBloodVisionTierChanged.Broadcast(State.Tier(),State.Tier()/4.f);
    for (int I=2; I<static_cast<int>(InjuryRules::Part::Count); ++I)
        if (Old.Wounds[I]!=State.Wounds[I]) OnBodyPartInjuryChanged.Broadcast(static_cast<EInjuryBodyPart>(I),State.Wounds[I]);
    const bool Before[]={Old.IsInjured(InjuryRules::Part::Torso),Old.LeftArm(),Old.RightArm(),Old.LeftLeg(),Old.RightLeg()};
    const bool After[]={State.IsInjured(InjuryRules::Part::Torso),State.LeftArm(),State.RightArm(),State.LeftLeg(),State.RightLeg()};
    for (int I=0;I<5;++I) if (Before[I]!=After[I]) OnRegionInjuryChanged.Broadcast(static_cast<EInjuryRegion>(I+2),After[I]);
    if (Old.Recoil()!=State.Recoil() || Old.ReloadDuration()!=State.ReloadDuration() || Old.MoveSpeed()!=State.MoveSpeed())
    {
        OnInjuryModifiersChanged.Broadcast(State.Recoil(),State.ReloadDuration(),State.MoveSpeed());
        OnGameplayModifiersChanged.Broadcast(State.ReloadDuration(),State.ReloadSpeed(),State.MoveSpeed());
    }
    if (Old.Bleeding!=State.Bleeding) OnBleedingChanged.Broadcast(State.Bleeding);
    if (Old.Health>0 && State.Health<=0) { EndFlash(); OnDeath.Broadcast(); }
}
void UInjuryComponent::ResetInjuries()
{
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(BleedTimer);
    EndFlash(); SyncTuning(); State.Reset(); ResetPartHealth(); BroadcastCurrentState(); OnReset.Broadcast();
}
void UInjuryComponent::BroadcastCurrentState()
{
    OnHealthChanged.Broadcast(State.Health,State.MaxHealth);
    OnBloodVisionTierChanged.Broadcast(State.Tier(),State.Tier()/4.f);
    for (int I=2; I<static_cast<int>(InjuryRules::Part::Count); ++I) OnBodyPartInjuryChanged.Broadcast(static_cast<EInjuryBodyPart>(I),State.Wounds[I]);
    for (int I=2;I<=6;++I) OnRegionInjuryChanged.Broadcast(static_cast<EInjuryRegion>(I),IsRegionInjured(static_cast<EInjuryRegion>(I)));
    OnInjuryModifiersChanged.Broadcast(State.Recoil(),State.ReloadDuration(),State.MoveSpeed());
    OnGameplayModifiersChanged.Broadcast(State.ReloadDuration(),State.ReloadSpeed(),State.MoveSpeed());
    OnBleedingChanged.Broadcast(State.Bleeding);
    OnHeadConditionChanged.Broadcast(HeadState);
    if (IsBlinded())
    {
        OnBlindRequested.Broadcast(HeadState,GetBlindOpacity(),GetBlindRemainingSeconds(),ActiveFadeCurve);
        if (IsFlashed()) OnFlashRequested.Broadcast(GetBlindOpacity(),GetBlindRemainingSeconds());
        else OnStunRequested.Broadcast(GetBlindOpacity(),GetBlindRemainingSeconds());
    }
    else { OnFlashEnded.Broadcast(); OnStunEnded.Broadcast(); }
}
bool UInjuryComponent::EvaluateFlashExplosion(FVector ExplosionLocation, AActor* FlashActor)
{
    return EvaluateBlindExplosion(ExplosionLocation,FlashActor,EInjuryHeadState::Flashbang);
}
bool UInjuryComponent::EvaluateBlindExplosion(FVector ExplosionLocation, AActor* FlashActor, EInjuryHeadState Type)
{
    const APawn* Pawn=Cast<APawn>(GetOwner());
    APlayerController* PC=Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    if ((Type!=EInjuryHeadState::Flashbang && Type!=EInjuryHeadState::StunGrenade) || !PC || !PC->PlayerCameraManager || !GetWorld() || IsDead() || ExplosionLocation.ContainsNaN()) return false;
    FVector Eye; FRotator Rotation; PC->GetPlayerViewPoint(Eye,Rotation);
    const FVector Delta=ExplosionLocation-Eye;
    if (Delta.Size()>FlashMaxDistance || !FMath::IsFinite(FlashMaxDistance) || FlashMaxDistance<=0) return false;
    // 使用玩家的真实投影，适配摄像机 FOV 轴约束、窗口比例和分屏矩形。
    const ULocalPlayer* LocalPlayer=PC->GetLocalPlayer();
    if (!LocalPlayer || !LocalPlayer->ViewportClient || !LocalPlayer->ViewportClient->Viewport) return false;
    FSceneViewProjectionData Projection;
    if (!LocalPlayer->GetProjectionData(LocalPlayer->ViewportClient->Viewport,Projection)) return false;
    const FIntRect Rect=Projection.GetConstrainedViewRect();
    FVector2D Pixel;
    if (!FSceneView::ProjectWorldToScreen(ExplosionLocation,Rect,Projection.ComputeViewProjectionMatrix(),Pixel)) return false;
    if (Pixel.X<Rect.Min.X || Pixel.X>Rect.Max.X || Pixel.Y<Rect.Min.Y || Pixel.Y>Rect.Max.Y) return false;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(InjuryFlashLOS),true,GetOwner());
    if (FlashActor) Params.AddIgnoredActor(FlashActor);
    FHitResult Obstacle;
    if (GetWorld()->LineTraceSingleByChannel(Obstacle,Eye,ExplosionLocation,ECC_Visibility,Params)) return false;
    TriggerBlindness(Type); return IsBlinded() && HeadState==Type;
}
void UInjuryComponent::TriggerFlashForTest() { TriggerBlindness(EInjuryHeadState::Flashbang); }
void UInjuryComponent::TriggerStunForTest() { TriggerBlindness(EInjuryHeadState::StunGrenade); }
void UInjuryComponent::TriggerBlindness(EInjuryHeadState Type)
{
    if ((Type!=EInjuryHeadState::Flashbang && Type!=EInjuryHeadState::StunGrenade) || IsDead() || !GetWorld()) return;
    const float ConfigDuration=Type==EInjuryHeadState::Flashbang ? FlashDurationSeconds : StunDurationSeconds;
    if (!FMath::IsFinite(ConfigDuration) || ConfigDuration<=0 || !FMath::IsFinite(BlindInitialOpacity)) return;
    // 同种连续道具保留较长剩余时间；换种道具先发出旧类型结束事件。
    const float Duration=FMath::Max(ConfigDuration,HeadState==Type ? GetBlindRemainingSeconds() : 0.f);
    if (IsBlinded() && HeadState!=Type) EndFlash();
    HeadState=Type; ActiveBlindDuration=Duration; ActiveBlindOpacity=FMath::Clamp(BlindInitialOpacity,0.f,1.f);
    ActiveEaseExponent=FMath::IsFinite(BlindEaseOutExponent) ? FMath::Max(1.f,BlindEaseOutExponent) : 2.f;
    ActiveFadeCurve=BlindFadeCurve;
    GetWorld()->GetTimerManager().SetTimer(FlashTimer,this,&UInjuryComponent::EndFlash,Duration,false);
    OnHeadConditionChanged.Broadcast(HeadState);
    OnBlindRequested.Broadcast(Type,ActiveBlindOpacity,Duration,ActiveFadeCurve);
    if (Type==EInjuryHeadState::Flashbang) OnFlashRequested.Broadcast(ActiveBlindOpacity,Duration);
    else OnStunRequested.Broadcast(ActiveBlindOpacity,Duration);
}
float UInjuryComponent::GetFlashRemainingSeconds() const
{
    return IsFlashed() ? GetBlindRemainingSeconds() : 0.f;
}
float UInjuryComponent::GetBlindRemainingSeconds() const
{
    return IsBlinded() && GetWorld() ? FMath::Max(0.f,GetWorld()->GetTimerManager().GetTimerRemaining(FlashTimer)) : 0.f;
}
float UInjuryComponent::GetBlindOpacity() const
{
    if (!IsBlinded() || ActiveBlindDuration<=0) return 0;
    const float Progress=FMath::Clamp(1.f-GetBlindRemainingSeconds()/ActiveBlindDuration,0.f,1.f);
    if (Progress>=1) return 0;
    const float Weight=ActiveFadeCurve ? ActiveFadeCurve->GetFloatValue(Progress) : FMath::Pow(1.f-Progress,ActiveEaseExponent);
    return ActiveBlindOpacity*(FMath::IsFinite(Weight) ? FMath::Clamp(Weight,0.f,1.f) : 0);
}
void UInjuryComponent::EndFlash()
{
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(FlashTimer);
    if (IsBlinded())
    {
        const auto Old=HeadState; HeadState=EInjuryHeadState::Normal; ActiveBlindDuration=0; ActiveFadeCurve=nullptr;
        OnHeadConditionChanged.Broadcast(HeadState); OnBlindEnded.Broadcast(Old);
        if (Old==EInjuryHeadState::Flashbang) OnFlashEnded.Broadcast(); else OnStunEnded.Broadcast();
    }
}
