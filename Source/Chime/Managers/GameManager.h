#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameManager.generated.h"

UCLASS()
class CHIME_API UGameManager : public UObject
{
	GENERATED_BODY()

public:
	
	static UGameManager* GetInstance();
};
