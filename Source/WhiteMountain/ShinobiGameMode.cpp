#include "ShinobiGameMode.h"
#include "ShinobiCharacter.h"
#include "ShinobiHUD.h"

AShinobiGameMode::AShinobiGameMode()
{
	DefaultPawnClass = AShinobiCharacter::StaticClass();
	HUDClass = AShinobiHUD::StaticClass();
}
