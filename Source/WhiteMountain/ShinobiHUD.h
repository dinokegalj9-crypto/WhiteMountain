#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ShinobiHUD.generated.h"

/** Debug-style HUD drawn with Canvas, so the prototype needs no UMG widgets. */
UCLASS()
class WHITEMOUNTAIN_API AShinobiHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	UPROPERTY(EditAnywhere, Category = "HUD")
	bool bShowControls = true;

private:
	void DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Color);
	void DrawGuardMarkers();
};
