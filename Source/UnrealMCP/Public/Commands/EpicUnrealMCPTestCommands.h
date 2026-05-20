// Automated Testing MCP Commands
#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Handler for automated testing commands
 * Supports creating FunctionalTest Blueprints and automation tests
 */
class FEpicUnrealMCPTestCommands
{
public:
	FEpicUnrealMCPTestCommands();

	/**
	 * Main command dispatcher
	 */
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
	// FunctionalTest creation and management
	TSharedPtr<FJsonObject> HandleCreateFunctionalTest(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleListAllTests(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetTestInfo(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetTestTimeout(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetTestDescription(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleDeleteTest(const TSharedPtr<FJsonObject>& Params);

	// Test map management
	TSharedPtr<FJsonObject> HandleCreateTestMap(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAddActorToTestMap(const TSharedPtr<FJsonObject>& Params);

	// Utilities
	TSharedPtr<FJsonObject> CreateErrorResponse(const FString& ErrorMessage);
	TSharedPtr<FJsonObject> CreateSuccessResponse(const FString& Message = TEXT(""));
};
