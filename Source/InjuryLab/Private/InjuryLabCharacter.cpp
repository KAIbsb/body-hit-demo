#include "InjuryLabCharacter.h"
#include "InjuryComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"
AInjuryLabCharacter::AInjuryLabCharacter()
{
    GetCapsuleComponent()->InitCapsuleSize(42,96);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);
    Injury=CreateDefaultSubobject<UInjuryComponent>(TEXT("Injury"));
    FirstPersonCamera=CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(0,0,64));
    FirstPersonCamera->bUsePawnControlRotation=true;
    FirstPersonCamera->SetFieldOfView(90);
    GetCharacterMovement()->MaxWalkSpeed=600;
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Manny(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
    static ConstructorHelpers::FClassFinder<UAnimInstance> Anim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
    if (Manny.Succeeded()) GetMesh()->SetSkeletalMesh(Manny.Object);
    if (Anim.Succeeded()) GetMesh()->SetAnimInstanceClass(Anim.Class);
    GetMesh()->SetRelativeLocation(FVector(0,0,-96));
    GetMesh()->SetRelativeRotation(FRotator(0,-90,0));
    GetMesh()->SetOwnerNoSee(true);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    GetMesh()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
}
void AInjuryLabCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("InjuryMoveForward"),this,&AInjuryLabCharacter::MoveForward);
    Input->BindAxis(TEXT("InjuryMoveRight"),this,&AInjuryLabCharacter::MoveRight);
    Input->BindAxis(TEXT("InjuryLookYaw"),this,&AInjuryLabCharacter::LookYaw);
    Input->BindAxis(TEXT("InjuryLookPitch"),this,&AInjuryLabCharacter::LookPitch);
    Input->BindAction(TEXT("InjuryJump"),IE_Pressed,this,&ACharacter::Jump);
    Input->BindAction(TEXT("InjuryJump"),IE_Released,this,&ACharacter::StopJumping);
}
void AInjuryLabCharacter::MoveForward(float V) { if (Controller) AddMovementInput(FRotationMatrix(FRotator(0,Controller->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::X),V); }
void AInjuryLabCharacter::MoveRight(float V) { if (Controller) AddMovementInput(FRotationMatrix(FRotator(0,Controller->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::Y),V); }
void AInjuryLabCharacter::LookYaw(float V) { if (const auto* PC=Cast<APlayerController>(Controller); PC && !PC->bShowMouseCursor) AddControllerYawInput(V); }
void AInjuryLabCharacter::LookPitch(float V) { if (const auto* PC=Cast<APlayerController>(Controller); PC && !PC->bShowMouseCursor) AddControllerPitchInput(V); }
