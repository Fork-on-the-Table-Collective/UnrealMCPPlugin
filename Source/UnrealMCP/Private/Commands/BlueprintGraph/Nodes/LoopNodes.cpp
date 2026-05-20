#include "Commands/BlueprintGraph/Nodes/LoopNodes.h"
#include "Commands/BlueprintGraph/Nodes/NodeCreatorUtils.h"
#include "K2Node_MacroInstance.h"
#include "EdGraphSchema_K2.h"
#include "Json.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"

namespace
{
	// Finds one of the standard macro graphs inside the StandardMacros blueprint library.
	// Returns nullptr if the library or named graph cannot be loaded.
	static UEdGraph* FindStandardMacroGraph(const FString& MacroName)
	{
		const FString LibraryPath = TEXT("/Engine/EditorBlueprintResources/StandardMacros");

		UBlueprint* MacroLibrary = Cast<UBlueprint>(
			StaticLoadObject(UBlueprint::StaticClass(), nullptr, *LibraryPath));
		if (!MacroLibrary)
		{
			UE_LOG(LogTemp, Warning, TEXT("LoopNodes: Failed to load StandardMacros library at %s"), *LibraryPath);
			return nullptr;
		}

		for (UEdGraph* Graph : MacroLibrary->MacroGraphs)
		{
			if (Graph && Graph->GetName().Equals(MacroName, ESearchCase::IgnoreCase))
			{
				return Graph;
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("LoopNodes: Macro '%s' not found in StandardMacros library"), *MacroName);
		return nullptr;
	}

	static UK2Node_MacroInstance* CreateMacroInstanceNode(UEdGraph* Graph, const FString& MacroName,
		const TSharedPtr<FJsonObject>& Params)
	{
		if (!Graph)
		{
			return nullptr;
		}

		UEdGraph* MacroGraph = FindStandardMacroGraph(MacroName);
		if (!MacroGraph)
		{
			return nullptr;
		}

		UK2Node_MacroInstance* MacroNode = NewObject<UK2Node_MacroInstance>(Graph);
		if (!MacroNode)
		{
			return nullptr;
		}

		MacroNode->SetMacroGraph(MacroGraph);

		double PosX = 0.0;
		double PosY = 0.0;
		FNodeCreatorUtils::ExtractNodePosition(Params, PosX, PosY);
		MacroNode->NodePosX = static_cast<int32>(PosX);
		MacroNode->NodePosY = static_cast<int32>(PosY);

		Graph->AddNode(MacroNode, false, false);
		MacroNode->CreateNewGuid();
		MacroNode->PostPlacedNewNode();
		FNodeCreatorUtils::InitializeK2Node(MacroNode, Graph);

		return MacroNode;
	}
}

UK2Node* FLoopNodeCreator::CreateForLoopNode(UEdGraph* Graph, const TSharedPtr<FJsonObject>& Params)
{
	if (!Graph || !Params.IsValid())
	{
		return nullptr;
	}

	// Standard Macro Library contains a "ForLoop" macro with inputs: First, Last
	// and outputs: Loop Body (exec), Index (int), Completed (exec)
	UK2Node_MacroInstance* MacroNode = CreateMacroInstanceNode(Graph, TEXT("ForLoop"), Params);
	if (!MacroNode)
	{
		return nullptr;
	}

	// Set default pin values if provided
	double StartIndex = 0.0;
	if (Params->TryGetNumberField(TEXT("start_index"), StartIndex))
	{
		UEdGraphPin* FirstPin = MacroNode->FindPin(TEXT("First"));
		if (FirstPin && FirstPin->Direction == EGPD_Input)
		{
			FirstPin->DefaultValue = FString::Printf(TEXT("%d"), static_cast<int32>(StartIndex));
		}
	}

	double EndIndex = 0.0;
	if (Params->TryGetNumberField(TEXT("end_index"), EndIndex))
	{
		UEdGraphPin* LastPin = MacroNode->FindPin(TEXT("Last"));
		if (LastPin && LastPin->Direction == EGPD_Input)
		{
			LastPin->DefaultValue = FString::Printf(TEXT("%d"), static_cast<int32>(EndIndex));
		}
	}

	return MacroNode;
}

UK2Node* FLoopNodeCreator::CreateWhileLoopNode(UEdGraph* Graph, const TSharedPtr<FJsonObject>& Params)
{
	if (!Graph || !Params.IsValid())
	{
		return nullptr;
	}

	// Standard Macro Library contains a "WhileLoop" macro with inputs: Condition (bool)
	// and outputs: Loop Body (exec), Completed (exec)
	UK2Node_MacroInstance* MacroNode = CreateMacroInstanceNode(Graph, TEXT("WhileLoop"), Params);
	return MacroNode;
}

UK2Node* FLoopNodeCreator::CreateForEachLoopNode(UEdGraph* Graph, const TSharedPtr<FJsonObject>& Params)
{
	if (!Graph || !Params.IsValid())
	{
		return nullptr;
	}

	// Standard Macro Library contains a "ForEachLoop" macro with Array input
	// and outputs: Loop Body (exec), Array Element (wildcard), Array Index (int), Completed (exec)
	UK2Node_MacroInstance* MacroNode = CreateMacroInstanceNode(Graph, TEXT("ForEachLoop"), Params);
	return MacroNode;
}
