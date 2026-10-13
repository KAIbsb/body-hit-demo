#pragma once
#include "CoreMinimal.h"
#include "InjuryComponent.h"
#include "InjuryEventProbe.generated.h"

// 自动化测试使用真实蓝图动态事件接收器，验证对外事件与参数。
UCLASS(Transient)
class UInjuryEventProbe : public UObject
{
    GENERATED_BODY()
public:
    int32 BlindRequests=0, BlindEnds=0, Regions=0, ModifierEvents=0, Deaths=0;
    EInjuryHeadState LastBlind=EInjuryHeadState::Normal, LastEnded=EInjuryHeadState::Normal;
    EInjuryRegion LastRegion=EInjuryRegion::Unknown;
    float Opacity=0, Duration=0, ReloadDuration=1, ReloadSpeed=1, MoveSpeed=1;
    UFUNCTION() void Blind(EInjuryHeadState Type,float Alpha,float Seconds,UCurveFloat* Curve) { ++BlindRequests; LastBlind=Type; Opacity=Alpha; Duration=Seconds; }
    UFUNCTION() void Ended(EInjuryHeadState Type) { ++BlindEnds; LastEnded=Type; }
    UFUNCTION() void Region(EInjuryRegion Part,bool bInjured) { ++Regions; LastRegion=Part; }
    UFUNCTION() void Modifiers(float ReloadTime,float ReloadRate,float Speed) { ++ModifierEvents; ReloadDuration=ReloadTime; ReloadSpeed=ReloadRate; MoveSpeed=Speed; }
    UFUNCTION() void Died() { ++Deaths; }
};
