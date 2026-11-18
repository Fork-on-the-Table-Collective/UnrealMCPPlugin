#include "Commands/BlueprintGraph/EventManager.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_Event.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EditorAssetLibrary.h"

TSharedPtr<FJsonObject> FEventManager::AddEventNode(const TSharedPtr<FJsonObject>& Params)
{
	// Validate parameters
	if (!Params.IsValid())
	{
		return CreateErrorResponse(TEXT("Invalid parameters"));
	}

	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString EventName;
	if (!Params->TryGetStringField(TEXT("event_name"), EventName))
	{
		return CreateErrorResponse(TEXT("Missing 'event_name' parameter"));
	}

	// Get optional position parameters
	FVector2D Position(0.0f, 0.0f);
	double PosX = 0.0, PosY = 0.0;
	if (Params->TryGetNumberField(TEXT("pos_x"), PosX))
	{
		Position.X = static_cast<float>(PosX);
	}
	if (Params->TryGetNumberField(TEXT("pos_y"), PosY))
	{
		Position.Y = static_cast<float>(PosY);
	}

	// Load the Blueprint
	UBlueprint* Blueprint = LoadBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	// Get the event graph (events can only exist in the event graph)
	if (Blueprint->UbergraphPages.Num() == 0)
	{
		return CreateErrorResponse(TEXT("Blueprint has no event graph"));
	}

	UEdGraph* Graph = Blueprint->UbergraphPages[0];
	if (!Graph)
	{
		return CreateErrorResponse(TEXT("Failed to get Blueprint event graph"));
	}

	// Create (or reuse) the override event node. Shared with FEpicUnrealMCPCommonUtils so every
	// event path sets bOverrideFunction, the declaring class and a node GUID the same way.
	FString CreateError;
	UK2Node_Event* EventNode = FEpicUnrealMCPCommonUtils::CreateEventNode(Graph, EventName, Position, &CreateError);
	if (!EventNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to create event node '%s': %s"), *EventName, *CreateError));
	}

	// Notify changes
	Graph->NotifyGraphChanged();
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	return CreateSuccessResponse(EventNode);
}

UBlueprint* FEventManager::LoadBlueprint(const FString& BlueprintName)
{
	// Try direct path first
	FString BlueprintPath = BlueprintName;

	// If no path prefix, assume /Game/Blueprints/
	if (!BlueprintPath.StartsWith(TEXT("/")))
	{
		BlueprintPath = TEXT("/Game/Blueprints/") + BlueprintPath;
	}

	// Add .Blueprint suffix if not present
	if (!BlueprintPath.Contains(TEXT(".")))
	{
		BlueprintPath += TEXT(".") + FPaths::GetBaseFilename(BlueprintPath);
	}

	// Try to load the Blueprint
	UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *BlueprintPath);

	// If not found, try with UEditorAssetLibrary
	if (!BP)
	{
		FString AssetPath = BlueprintPath;
		if (UEditorAssetLibrary::DoesAssetExist(AssetPath))
		{
			UObject* Asset = UEditorAssetLibrary::LoadAsset(AssetPath);
			BP = Cast<UBlueprint>(Asset);
		}
	}

	return BP;
}

TSharedPtr<FJsonObject> FEventManager::CreateSuccessResponse(const UK2Node_Event* EventNode)
{
	TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
	Response->SetBoolField(TEXT("success"), true);
	Response->SetStringField(TEXT("node_id"), EventNode->NodeGuid.ToString());
	Response->SetStringField(TEXT("node_name"), EventNode->GetName());
	Response->SetStringField(TEXT("event_name"), EventNode->EventReference.GetMemberName().ToString());
	Response->SetBoolField(TEXT("is_override"), EventNode->bOverrideFunction);
	if (const UClass* DeclaringClass = EventNode->EventReference.GetMemberParentClass())
	{
		Response->SetStringField(TEXT("declaring_class"), DeclaringClass->GetPathName());
	}
	Response->SetNumberField(TEXT("pos_x"), EventNode->NodePosX);
	Response->SetNumberField(TEXT("pos_y"), EventNode->NodePosY);
	return Response;
}

TSharedPtr<FJsonObject> FEventManager::CreateErrorResponse(const FString& ErrorMessage)
{
	TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
	Response->SetBoolField(TEXT("success"), false);
	Response->SetStringField(TEXT("error"), ErrorMessage);
	return Response;
}
