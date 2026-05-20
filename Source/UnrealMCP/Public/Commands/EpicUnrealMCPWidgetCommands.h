#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for UMG/Widget Designer MCP commands
 * Handles widget creation, modification, inspection, and deletion
 */
class UNREALMCP_API FEpicUnrealMCPWidgetCommands
{
public:
	FEpicUnrealMCPWidgetCommands();

	// Handle widget commands
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
	// Widget Blueprint management
	TSharedPtr<FJsonObject> HandleCreateWidgetBlueprint(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleDeleteWidgetBlueprint(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetWidgetBlueprintInfo(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleListAllWidgets(const TSharedPtr<FJsonObject>& Params);

	// Widget Designer hierarchy management
	TSharedPtr<FJsonObject> HandleGetWidgetTree(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAddWidgetToTree(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleRemoveWidgetFromTree(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleReparentWidget(const TSharedPtr<FJsonObject>& Params);

	// Widget property manipulation
	TSharedPtr<FJsonObject> HandleGetWidgetProperties(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetWidgetProperty(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetWidgetTransform(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetWidgetAnchors(const TSharedPtr<FJsonObject>& Params);

	// Widget slot properties
	TSharedPtr<FJsonObject> HandleGetSlotProperties(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetSlotProperty(const TSharedPtr<FJsonObject>& Params);

	// Utility functions
	class UWidgetBlueprint* LoadWidgetBlueprint(const FString& WidgetPath);
	class UWidget* FindWidgetInTree(class UWidgetBlueprint* WidgetBP, const FString& WidgetName);
	TSharedPtr<FJsonObject> WidgetToJson(class UWidget* Widget);
	TSharedPtr<FJsonObject> CreateErrorResponse(const FString& ErrorMessage);
	TSharedPtr<FJsonObject> CreateSuccessResponse(const FString& Message);
};
