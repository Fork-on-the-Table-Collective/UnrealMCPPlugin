// Automated Testing MCP Commands Implementation
#include "Commands/EpicUnrealMCPTestCommands.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "Editor.h"
#include "Dom/JsonObject.h"
#include "FunctionalTest.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "PackageTools.h"
#include "Factories/BlueprintFactory.h"
#include "FileHelpers.h"
#include "UObject/SavePackage.h"

FEpicUnrealMCPTestCommands::FEpicUnrealMCPTestCommands()
{
}

TSharedPtr<FJsonObject> FEpicUnrealMCPTestCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandType == TEXT("create_functional_test"))
	{
		return HandleCreateFunctionalTest(Params);
	}
	else if (CommandType == TEXT("list_all_tests"))
	{
		return HandleListAllTests(Params);
	}
	else if (CommandType == TEXT("get_test_info"))
	{
		return HandleGetTestInfo(Params);
	}
	else if (CommandType == TEXT("set_test_timeout"))
	{
		return HandleSetTestTimeout(Params);
	}
	else if (CommandType == TEXT("set_test_description"))
	{
		return HandleSetTestDescription(Params);
	}
	else if (CommandType == TEXT("delete_test"))
	{
		return HandleDeleteTest(Params);
	}
	else if (CommandType == TEXT("create_test_map"))
	{
		return HandleCreateTestMap(Params);
	}
	else if (CommandType == TEXT("add_actor_to_test_map"))
	{
		return HandleAddActorToTestMap(Params);
	}

	return CreateErrorResponse(TEXT("Unknown test command"));
}

// Create a FunctionalTest Blueprint
TSharedPtr<FJsonObject> FEpicUnrealMCPTestCommands::HandleCreateFunctionalTest(const TSharedPtr<FJsonObject>& Params)
{
	FString TestName = Params->GetStringField(TEXT("test_name"));
	FString TestMap = Params->HasField(TEXT("test_map")) ? Params->GetStringField(TEXT("test_map")) : TEXT("");
	FString Description = Params->HasField(TEXT("description")) ? Params->GetStringField(TEXT("description")) : TEXT("");
	float TimeLimit = Params->HasField(TEXT("time_limit")) ? static_cast<float>(Params->GetNumberField(TEXT("time_limit"))) : 60.0f;

	// Validate path format
	if (!TestName.StartsWith(TEXT("/Game/")))
	{
		return CreateErrorResponse(TEXT("Test name must start with /Game/"));
	}

	// Check if already exists
	if (UEditorAssetLibrary::DoesAssetExist(TestName))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Test already exists: %s"), *TestName));
	}

	// Get package path
	FString PackageName = TestName;
	FString AssetName = FPaths::GetBaseFilename(PackageName);

	// Create package
	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		return CreateErrorResponse(TEXT("Failed to create package"));
	}

	// Create Blueprint with AFunctionalTest as parent
	UClass* ParentClass = AFunctionalTest::StaticClass();
	
	UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
	Factory->ParentClass = ParentClass;

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	UBlueprint* NewBlueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(
		UBlueprint::StaticClass(),
		Package,
		*AssetName,
		RF_Public | RF_Standalone,
		nullptr,
		GWarn
	));

	if (!NewBlueprint)
	{
		return CreateErrorResponse(TEXT("Failed to create FunctionalTest Blueprint"));
	}

	// Get the generated CDO
	AFunctionalTest* TestCDO = Cast<AFunctionalTest>(NewBlueprint->GeneratedClass->GetDefaultObject());
	if (TestCDO)
	{
		// Set default properties
		TestCDO->TimeLimit = TimeLimit;
		TestCDO->Description = Description;
	}

	// Compile the Blueprint
	FKismetEditorUtilities::CompileBlueprint(NewBlueprint);

	// Mark package dirty and save
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewBlueprint);

	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("status"), TEXT("success"));
	Response->SetStringField(TEXT("test_name"), TestName);
	Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Created FunctionalTest: %s"), *TestName));
	return Response;
}

// List all FunctionalTest assets
TSharedPtr<FJsonObject> FEpicUnrealMCPTestCommands::HandleListAllTests(const TSharedPtr<FJsonObject>& Params)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Find all Blueprints
	TArray<FAssetData> BlueprintAssets;
	AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), BlueprintAssets, true);

	TArray<TSharedPtr<FJsonValue>> TestsList;

	for (const FAssetData& Asset : BlueprintAssets)
	{
		// Load Blueprint to check parent class
		UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset());
		if (Blueprint && Blueprint->GeneratedClass)
		{
			// Check if it's a FunctionalTest
			if (Blueprint->GeneratedClass->IsChildOf(AFunctionalTest::StaticClass()))
			{
				TSharedPtr<FJsonObject> TestInfo = MakeShared<FJsonObject>();
				TestInfo->SetStringField(TEXT("test_name"), Asset.GetObjectPathString());
				TestInfo->SetStringField(TEXT("package_path"), Asset.PackagePath.ToString());
				
				// Get test properties if possible
				AFunctionalTest* TestCDO = Cast<AFunctionalTest>(Blueprint->GeneratedClass->GetDefaultObject());
				if (TestCDO)
				{
					TestInfo->SetStringField(TEXT("description"), TestCDO->Description);
					TestInfo->SetNumberField(TEXT("time_limit"), TestCDO->TimeLimit);
				}

				TestsList.Add(MakeShared<FJsonValueObject>(TestInfo));
			}
		}
	}

	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("status"), TEXT("success"));
	Response->SetArrayField(TEXT("tests"), TestsList);
	Response->SetNumberField(TEXT("count"), TestsList.Num());
	return Response;
}

// Get detailed test information
TSharedPtr<FJsonObject> FEpicUnrealMCPTestCommands::HandleGetTestInfo(const TSharedPtr<FJsonObject>& Params)
{
	FString TestName = Params->GetStringField(TEXT("test_name"));

	UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(TestName);
	UBlueprint* Blueprint = Cast<UBlueprint>(LoadedAsset);

	if (!Blueprint || !Blueprint->GeneratedClass)
	{
		return CreateErrorResponse(TEXT("Failed to load test Blueprint"));
	}

	if (!Blueprint->GeneratedClass->IsChildOf(AFunctionalTest::StaticClass()))
	{
		return CreateErrorResponse(TEXT("Asset is not a FunctionalTest"));
	}

	AFunctionalTest* TestCDO = Cast<AFunctionalTest>(Blueprint->GeneratedClass->GetDefaultObject());
	if (!TestCDO)
	{
		return CreateErrorResponse(TEXT("Failed to get test CDO"));
	}

	TSharedPtr<FJsonObject> TestInfo = MakeShared<FJsonObject>();
	TestInfo->SetStringField(TEXT("test_name"), TestName);
	TestInfo->SetStringField(TEXT("description"), TestCDO->Description);
	TestInfo->SetNumberField(TEXT("time_limit"), TestCDO->TimeLimit);
	TestInfo->SetStringField(TEXT("author"), TestCDO->Author);
	TestInfo->SetBoolField(TEXT("is_enabled"), TestCDO->IsEnabled());

	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("status"), TEXT("success"));
	Response->SetObjectField(TEXT("test_info"), TestInfo);
	return Response;
}

// Set test timeout
TSharedPtr<FJsonObject> FEpicUnrealMCPTestCommands::HandleSetTestTimeout(const TSharedPtr<FJsonObject>& Params)
{
	FString TestName = Params->GetStringField(TEXT("test_name"));
	float TimeLimit = static_cast<float>(Params->GetNumberField(TEXT("time_limit")));

	UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(TestName);
	UBlueprint* Blueprint = Cast<UBlueprint>(LoadedAsset);

	if (!Blueprint || !Blueprint->GeneratedClass)
	{
		return CreateErrorResponse(TEXT("Failed to load test Blueprint"));
	}

	AFunctionalTest* TestCDO = Cast<AFunctionalTest>(Blueprint->GeneratedClass->GetDefaultObject());
	if (!TestCDO)
	{
		return CreateErrorResponse(TEXT("Not a FunctionalTest"));
	}

	TestCDO->TimeLimit = TimeLimit;
	TestCDO->Modify();

	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	return CreateSuccessResponse(FString::Printf(TEXT("Set timeout to %.1f seconds"), TimeLimit));
}

// Set test description
TSharedPtr<FJsonObject> FEpicUnrealMCPTestCommands::HandleSetTestDescription(const TSharedPtr<FJsonObject>& Params)
{
	FString TestName = Params->GetStringField(TEXT("test_name"));
	FString Description = Params->GetStringField(TEXT("description"));

	UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(TestName);
	UBlueprint* Blueprint = Cast<UBlueprint>(LoadedAsset);

	if (!Blueprint || !Blueprint->GeneratedClass)
	{
		return CreateErrorResponse(TEXT("Failed to load test Blueprint"));
	}

	AFunctionalTest* TestCDO = Cast<AFunctionalTest>(Blueprint->GeneratedClass->GetDefaultObject());
	if (!TestCDO)
	{
		return CreateErrorResponse(TEXT("Not a FunctionalTest"));
	}

	TestCDO->Description = Description;
	TestCDO->Modify();

	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	return CreateSuccessResponse(TEXT("Description updated"));
}

// Delete test
TSharedPtr<FJsonObject> FEpicUnrealMCPTestCommands::HandleDeleteTest(const TSharedPtr<FJsonObject>& Params)
{
	FString TestName = Params->GetStringField(TEXT("test_name"));

	if (!UEditorAssetLibrary::DoesAssetExist(TestName))
	{
		return CreateErrorResponse(TEXT("Test does not exist"));
	}

	bool bSuccess = UEditorAssetLibrary::DeleteAsset(TestName);
	
	if (bSuccess)
	{
		return CreateSuccessResponse(TEXT("Test deleted"));
	}
	else
	{
		return CreateErrorResponse(TEXT("Failed to delete test"));
	}
}

// Create a test map
TSharedPtr<FJsonObject> FEpicUnrealMCPTestCommands::HandleCreateTestMap(const TSharedPtr<FJsonObject>& Params)
{
	FString MapName = Params->GetStringField(TEXT("map_name"));

	if (!MapName.StartsWith(TEXT("/Game/")))
	{
		return CreateErrorResponse(TEXT("Map name must start with /Game/"));
	}

	if (UEditorAssetLibrary::DoesAssetExist(MapName))
	{
		return CreateErrorResponse(TEXT("Map already exists"));
	}

	// Create a new blank world
	UWorld* NewWorld = UWorld::CreateWorld(EWorldType::Editor, false);
	if (!NewWorld)
	{
		return CreateErrorResponse(TEXT("Failed to create world"));
	}

	// Save the world as a map
	FString PackageName = MapName;
	UPackage* Package = CreatePackage(*PackageName);
	NewWorld->Rename(*FPaths::GetBaseFilename(PackageName), Package);

	// Save package
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetMapPackageExtension());
	
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_None;
	bool bSaved = UPackage::SavePackage(Package, NewWorld, *PackageFileName, SaveArgs);

	if (bSaved)
	{
		FAssetRegistryModule::AssetCreated(NewWorld);
		return CreateSuccessResponse(FString::Printf(TEXT("Created test map: %s"), *MapName));
	}
	else
	{
		return CreateErrorResponse(TEXT("Failed to save test map"));
	}
}

// Add actor to test map
TSharedPtr<FJsonObject> FEpicUnrealMCPTestCommands::HandleAddActorToTestMap(const TSharedPtr<FJsonObject>& Params)
{
	FString MapName = Params->GetStringField(TEXT("map_name"));
	FString ActorPath = Params->GetStringField(TEXT("actor_path"));
	
	TArray<double> LocationArray;
	if (Params->HasField(TEXT("location")))
	{
		const TArray<TSharedPtr<FJsonValue>>* LocationValues;
		if (Params->TryGetArrayField(TEXT("location"), LocationValues))
		{
			for (const TSharedPtr<FJsonValue>& Val : *LocationValues)
			{
				LocationArray.Add(Val->AsNumber());
			}
		}
	}

	FVector Location = LocationArray.Num() >= 3 ? 
		FVector(LocationArray[0], LocationArray[1], LocationArray[2]) : FVector::ZeroVector;

	// This would need to load the map, spawn the actor, and save
	// For now, return a placeholder
	return CreateErrorResponse(TEXT("Add actor to test map not yet fully implemented - use standard spawn commands after loading the map"));
}

// Utility: Create error response
TSharedPtr<FJsonObject> FEpicUnrealMCPTestCommands::CreateErrorResponse(const FString& ErrorMessage)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("status"), TEXT("error"));
	Response->SetStringField(TEXT("error"), ErrorMessage);
	return Response;
}

// Utility: Create success response
TSharedPtr<FJsonObject> FEpicUnrealMCPTestCommands::CreateSuccessResponse(const FString& Message)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("status"), TEXT("success"));
	if (!Message.IsEmpty())
	{
		Response->SetStringField(TEXT("message"), Message);
	}
	return Response;
}
