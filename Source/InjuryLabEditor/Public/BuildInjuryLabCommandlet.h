#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "BuildInjuryLabCommandlet.generated.h"
UCLASS()
class UBuildInjuryLabCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UBuildInjuryLabCommandlet();
    virtual int32 Main(const FString& Params) override;
};
