#include "GameManager.h"
#include "Engine/Engine.h"

UGameManager* UGameManager::GetInstance() 
{
	if (GEngine) 
	{
		UGameManager* instance = Cast<UGameManager>(GEngine->GameSingleton);
		return instance;
	}

	return nullptr;
}
