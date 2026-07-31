#include "Commands/BlueprintGraph/Nodes/NodeCreatorUtils.h"
#include "K2Node.h"
#include "EdGraph/EdGraph.h"
#include "Json.h"

bool FNodeCreatorUtils::InitializeK2Node(UK2Node* Node, UEdGraph* Graph)
{
	// Basic checks
	if (!Node || !Graph)
	{
		return false;
	}

	// 1. Allocate default pins
	Node->AllocateDefaultPins();

	// 2. Reconstruct the node (notifies Unreal of changes)
	Node->ReconstructNode();

	// 3. Notify the graph that something changed
	Graph->NotifyGraphChanged();

	return true;
}

void FNodeCreatorUtils::ExtractNodePosition(const TSharedPtr<FJsonObject>& Params, double& OutX, double& OutY)
{
	// Initialize default values
	OutX = 0.0;
	OutY = 0.0;

	// Check that Params is valid
	if (!Params.IsValid())
	{
		return;
	}

	// Try to get pos_x
	if (!Params->TryGetNumberField(TEXT("pos_x"), OutX))
	{
		OutX = 0.0;
	}

	// Try to get pos_y
	if (!Params->TryGetNumberField(TEXT("pos_y"), OutY))
	{
		OutY = 0.0;
	}
}

UClass* FNodeCreatorUtils::ResolveClassByName(const FString& ClassName)
{
	if (ClassName.IsEmpty())
	{
		return nullptr;
	}

	// 1. Exact object path, e.g. "/Script/ViridianCo.V_InteractionComponent".
	if (UClass* Found = Cast<UClass>(StaticFindObject(UClass::StaticClass(), nullptr, *ClassName)))
	{
		return Found;
	}

	// 2. Blueprint asset path: the generated class carries a "_C" suffix.
	if (ClassName.StartsWith(TEXT("/")) && !ClassName.EndsWith(TEXT("_C")))
	{
		FString GeneratedPath = ClassName;
		if (!GeneratedPath.Contains(TEXT(".")))
		{
			// "/Game/Path/BP_Foo" -> "/Game/Path/BP_Foo.BP_Foo"
			FString AssetName;
			GeneratedPath.Split(TEXT("/"), nullptr, &AssetName, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
			GeneratedPath = FString::Printf(TEXT("%s.%s"), *ClassName, *AssetName);
		}

		GeneratedPath.Append(TEXT("_C"));
		if (UClass* Found = Cast<UClass>(StaticFindObject(UClass::StaticClass(), nullptr, *GeneratedPath)))
		{
			return Found;
		}
	}

	// 3. Bare class name, e.g. "V_InteractionComponent" or "AActor".
	return UClass::TryFindTypeSlow<UClass>(ClassName);
}
