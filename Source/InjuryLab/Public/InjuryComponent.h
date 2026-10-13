#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InjuryRules.h"
#include "InjuryComponent.generated.h"
class UCurveFloat;

// 枚举顺序与纯规则保持一致；Unknown 不会误当躯干。
UENUM(BlueprintType)
enum class EInjuryBodyPart : uint8
{
    Unknown, Head, Torso, LeftUpperArm, RightUpperArm, LeftForearm, RightForearm,
    LeftHand, RightHand, LeftThigh, RightThigh, LeftCalf, RightCalf, LeftFoot, RightFoot
};

UENUM(BlueprintType)
enum class EInjuryRegion : uint8 { Unknown, Head, Torso, LeftArm, RightArm, LeftLeg, RightLeg };
UENUM(BlueprintType)
enum class EInjuryHeadState : uint8 { Normal, Flashbang, StunGrenade };
UENUM(BlueprintType)
enum class EInjuryStackingPolicy : uint8 { Unified, StackBothSides };

// 对外使用的六区状态：头部是道具状态，其他五区是受损布尔值。
USTRUCT(BlueprintType)
struct FInjuryRegionStates
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Injury") EInjuryHeadState Head = EInjuryHeadState::Normal;
    UPROPERTY(BlueprintReadOnly, Category="Injury") bool bTorsoInjured = false;
    UPROPERTY(BlueprintReadOnly, Category="Injury") bool bLeftArmInjured = false;
    UPROPERTY(BlueprintReadOnly, Category="Injury") bool bRightArmInjured = false;
    UPROPERTY(BlueprintReadOnly, Category="Injury") bool bLeftLegInjured = false;
    UPROPERTY(BlueprintReadOnly, Category="Injury") bool bRightLegInjured = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInjuryHealthEvent, float, Health, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInjuryTierEvent, int32, Tier, float, Strength);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInjuryPartEvent, EInjuryBodyPart, Part, bool, bInjured);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FInjuryModifierEvent, float, RecoilMultiplier, float, ReloadDurationMultiplier, float, MoveSpeedMultiplier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInjuryFlashEvent, float, WhiteStrength, float, DurationSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInjuryBleedEvent, bool, bBleeding);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInjurySimpleEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInjuryRegionEvent, EInjuryRegion, Region, bool, bInjured);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInjuryHeadEvent, EInjuryHeadState, HeadState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FInjuryBlindEvent, EInjuryHeadState, Type, float, InitialOpacity, float, DurationSeconds, UCurveFloat*, FadeCurve);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FInjuryGameplayModifierEvent, float, ReloadDurationMultiplier, float, ReloadSpeedMultiplier, float, MoveSpeedMultiplier);

UCLASS(ClassGroup=(Injury), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class INJURYLAB_API UInjuryComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UInjuryComponent();
    // 所有可调数值均可由蓝图/组件详情编辑。头部不接受受伤入口。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Injury|Tuning", meta=(ClampMin="1")) float MaximumHealth = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Injury|Tuning", meta=(ClampMin="0")) float ArmBulletDamage = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Injury|Tuning", meta=(ClampMin="0")) float LegBulletDamage = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Injury|Bleeding", meta=(ClampMin="0.01")) float BleedIntervalSeconds = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Injury|Bleeding", meta=(ClampMin="0")) float BleedDamagePerTick = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Injury|Bleeding", meta=(ClampMin="1")) float BleedMinimumHealth = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Injury|Tuning", meta=(ClampMin="1")) float InjuredReloadDurationMultiplier = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Injury|Tuning", meta=(ClampMin="0", ClampMax="1")) float InjuredMoveSpeedMultiplier = 0.6f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Injury|Tuning") EInjuryStackingPolicy ArmStackingPolicy = EInjuryStackingPolicy::Unified;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Injury|Tuning") EInjuryStackingPolicy LegStackingPolicy = EInjuryStackingPolicy::Unified;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Injury|Flash", meta=(ClampMin="0.01")) float FlashDurationSeconds = 3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Injury|Flash", meta=(ClampMin="0.01")) float StunDurationSeconds = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Injury|Flash", meta=(ClampMin="0", ClampMax="1")) float BlindInitialOpacity = 1;
    // 归一化曲线：横轴 0–1 为时间进度，纵轴 1–0 为剩余透明度；空值使用 Ease Out。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Injury|Flash") TObjectPtr<UCurveFloat> BlindFadeCurve;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Injury|Flash", meta=(ClampMin="1")) float BlindEaseOutExponent = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Injury|Flash", meta=(ClampMin="1")) float FlashMaxDistance = 2000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Injury|Hit") TMap<FName, EInjuryBodyPart> BoneOverrides;

    UPROPERTY(BlueprintAssignable, Category="Injury|Events") FInjuryHealthEvent OnHealthChanged;
    UPROPERTY(BlueprintAssignable, Category="Injury|Events") FInjuryTierEvent OnBloodVisionTierChanged;
    UPROPERTY(BlueprintAssignable, Category="Injury|Events") FInjuryPartEvent OnBodyPartInjuryChanged;
    UPROPERTY(BlueprintAssignable, Category="Injury|Events") FInjuryModifierEvent OnInjuryModifiersChanged;
    UPROPERTY(BlueprintAssignable, Category="Injury|Events") FInjuryFlashEvent OnFlashRequested;
    UPROPERTY(BlueprintAssignable, Category="Injury|Events") FInjurySimpleEvent OnFlashEnded;
    UPROPERTY(BlueprintAssignable, Category="Injury|Events") FInjuryBleedEvent OnBleedingChanged;
    UPROPERTY(BlueprintAssignable, Category="Injury|Events") FInjurySimpleEvent OnDeath;
    UPROPERTY(BlueprintAssignable, Category="Injury|Events") FInjurySimpleEvent OnReset;
    UPROPERTY(BlueprintAssignable, Category="Injury|Events") FInjuryRegionEvent OnRegionInjuryChanged;
    UPROPERTY(BlueprintAssignable, Category="Injury|Events") FInjuryHeadEvent OnHeadConditionChanged;
    UPROPERTY(BlueprintAssignable, Category="Injury|Events") FInjuryBlindEvent OnBlindRequested;
    UPROPERTY(BlueprintAssignable, Category="Injury|Events") FInjuryHeadEvent OnBlindEnded;
    UPROPERTY(BlueprintAssignable, Category="Injury|Events") FInjuryFlashEvent OnStunRequested;
    UPROPERTY(BlueprintAssignable, Category="Injury|Events") FInjurySimpleEvent OnStunEnded;
    UPROPERTY(BlueprintAssignable, Category="Injury|Events") FInjuryGameplayModifierEvent OnGameplayModifiersChanged;

    UFUNCTION(BlueprintCallable, Category="Injury|Hit") bool ApplyDamageToPart(EInjuryRegion Part, float DamageAmount = 0);
    UFUNCTION(BlueprintPure, Category="Injury|State") FInjuryRegionStates GetRegionStates() const;
    UFUNCTION(BlueprintPure, Category="Injury|State") bool IsRegionInjured(EInjuryRegion Region) const;
    UFUNCTION(BlueprintPure, Category="Injury|Hit") EInjuryRegion ResolveRegion(FName BoneName) const;

    UFUNCTION(BlueprintCallable, Category="Injury|Hit", meta=(ReturnDisplayName="Accepted")) bool ApplyBulletHit(EInjuryBodyPart Part);
    UFUNCTION(BlueprintCallable, Category="Injury|Hit") bool ApplyBulletHitFromHit(const FHitResult& Hit);
    UFUNCTION(BlueprintPure, Category="Injury|Hit") EInjuryBodyPart ResolveBodyPart(FName BoneName) const;
    UFUNCTION(BlueprintCallable, Category="Injury|Health") void ApplyHealthDamage(float Amount);
    UFUNCTION(BlueprintCallable, Category="Injury|Health") void TestDamage25();
    UFUNCTION(BlueprintCallable, Category="Injury|Recovery") void ResetInjuries();
    UFUNCTION(BlueprintCallable, Category="Injury|Recovery") void StopBleeding();
    UFUNCTION(BlueprintCallable, Category="Injury|Recovery") void TreatTorso(float HealAmount = 0);
    UFUNCTION(BlueprintCallable, Category="Injury|Bleeding") void ConfigureBleeding(float IntervalSeconds, float DamagePerTick, float MinimumHealth);
    // 正式闪光入口：以玩家摄像机为准，检查屏幕范围、距离和遮挡。
    UFUNCTION(BlueprintCallable, Category="Injury|Flash") bool EvaluateFlashExplosion(FVector ExplosionLocation, AActor* FlashActor);
    // 实验/已经完成可见性判定的调用者使用此入口；不产生头部伤口。
    UFUNCTION(BlueprintCallable, Category="Injury|Flash") void TriggerFlashForTest();
    UFUNCTION(BlueprintCallable, Category="Injury|Flash") void TriggerStunForTest();
    UFUNCTION(BlueprintCallable, Category="Injury|Flash") void TriggerBlindness(EInjuryHeadState Type);
    UFUNCTION(BlueprintCallable, Category="Injury|Flash") bool EvaluateBlindExplosion(FVector ExplosionLocation, AActor* SourceActor, EInjuryHeadState Type);
    UFUNCTION(BlueprintCallable, Category="Injury|Events") void BroadcastCurrentState();
    UFUNCTION(BlueprintPure, Category="Injury|State") float GetHealth() const { return State.Health; }
    UFUNCTION(BlueprintPure, Category="Injury|State") float GetMaxHealth() const { return State.MaxHealth; }
    UFUNCTION(BlueprintPure, Category="Injury|State") float GetHealthPercent() const { return State.MaxHealth>0 ? FMath::Clamp(State.Health/State.MaxHealth,0.f,1.f) : 0; }
    UFUNCTION(BlueprintPure, Category="Injury|State") int32 GetBloodVisionTier() const { return State.Tier(); }
    UFUNCTION(BlueprintPure, Category="Injury|State") bool IsBodyPartInjured(EInjuryBodyPart Part) const;
    UFUNCTION(BlueprintPure, Category="Injury|State") bool IsBleeding() const { return State.Bleeding; }
    UFUNCTION(BlueprintPure, Category="Injury|State") bool IsDead() const { return State.Health <= 0; }
    UFUNCTION(BlueprintPure, Category="Injury|State") bool IsFlashed() const { return HeadState==EInjuryHeadState::Flashbang; }
    UFUNCTION(BlueprintPure, Category="Injury|State") bool IsBlinded() const { return HeadState!=EInjuryHeadState::Normal; }
    UFUNCTION(BlueprintPure, Category="Injury|State") EInjuryHeadState GetHeadState() const { return HeadState; }
    UFUNCTION(BlueprintPure, Category="Injury|State") float GetBlindRemainingSeconds() const;
    UFUNCTION(BlueprintPure, Category="Injury|State") float GetBlindOpacity() const;
    UFUNCTION(BlueprintPure, Category="Injury|State") float GetFlashRemainingSeconds() const;
    UFUNCTION(BlueprintPure, Category="Injury|State") float GetRecoilMultiplier() const { return State.Recoil(); }
    UFUNCTION(BlueprintPure, Category="Injury|State") float GetReloadDurationMultiplier() const { return State.ReloadDuration(); }
    UFUNCTION(BlueprintPure, Category="Injury|State") float GetReloadSpeedMultiplier() const { return State.ReloadSpeed(); }
    UFUNCTION(BlueprintPure, Category="Injury|State") float GetMoveSpeedMultiplier() const { return State.MoveSpeed(); }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    InjuryRules::State State;
    FTimerHandle BleedTimer, FlashTimer;
    EInjuryHeadState HeadState = EInjuryHeadState::Normal;
    float ActiveBleedInterval = 0.5f, ActiveBlindDuration = 0, ActiveBlindOpacity = 0, ActiveEaseExponent = 2;
    UPROPERTY() TObjectPtr<UCurveFloat> ActiveFadeCurve;
    static EInjuryRegion ToRegion(EInjuryBodyPart Part);
    bool ApplyPartDamage(EInjuryBodyPart Part, float Amount);
    void SyncTuning();
    void PublishChanges(const InjuryRules::State& Old);
    void HandleBleedTick();
    void EndFlash();
    UFUNCTION() void HandlePointDamage(AActor* DamagedActor, float Damage, AController* Instigator, FVector HitLocation, UPrimitiveComponent* HitComponent, FName BoneName, FVector ShotDirection, const UDamageType* DamageType, AActor* DamageCauser);
};
