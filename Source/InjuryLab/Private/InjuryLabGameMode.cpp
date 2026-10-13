#include "InjuryLabGameMode.h"
#include "InjuryLabCharacter.h"
#include "InjuryLabPlayerController.h"
AInjuryLabGameMode::AInjuryLabGameMode()
{
    DefaultPawnClass=AInjuryLabCharacter::StaticClass();
    PlayerControllerClass=AInjuryLabPlayerController::StaticClass();
}
