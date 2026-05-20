#include "Commands/BlueprintGraph/NodeLayoutManager.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Kismet2/BlueprintEditorUtils.h"

namespace
{
	/** Find a function graph by name in the blueprint. Falls back to event graph if FunctionName is empty. */
	static UEdGraph* ResolveGraph(UBlueprint* BP, const FString& FunctionName)
	{
		if (!BP)
		{
			return nullptr;
		}

		if (FunctionName.IsEmpty())
		{
			return BP->UbergraphPages.Num() > 0 ? BP->UbergraphPages[0] : nullptr;
		}

		for (UEdGraph* Graph : BP->FunctionGraphs)
		{
			if (Graph && Graph->GetName().Equals(FunctionName, ESearchCase::IgnoreCase))
			{
				return Graph;
			}
		}

		// Partial match
		for (UEdGraph* Graph : BP->FunctionGraphs)
		{
			if (Graph && Graph->GetName().Contains(FunctionName))
			{
				return Graph;
			}
		}

		return nullptr;
	}
}

TSharedPtr<FJsonObject> FNodeLayoutManager::ArrangeNodesAuto(const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid parameters"));
	}

	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name'"));
	}

	UBlueprint* BP = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!BP)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	FString FunctionName;
	Params->TryGetStringField(TEXT("function_name"), FunctionName);

	UEdGraph* Graph = ResolveGraph(BP, FunctionName);
	if (!Graph)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
	}

	double SpacingX = 300.0;
	double SpacingY = 200.0;
	Params->TryGetNumberField(TEXT("spacing_x"), SpacingX);
	Params->TryGetNumberField(TEXT("spacing_y"), SpacingY);

	// Simple grid layout: place nodes left-to-right, top-to-bottom
	const int32 Cols = FMath::Max(1, FMath::CeilToInt(FMath::Sqrt(static_cast<float>(Graph->Nodes.Num()))));
	int32 Col = 0;
	int32 Row = 0;

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (!Node)
		{
			continue;
		}

		Node->NodePosX = static_cast<int32>(Col * SpacingX);
		Node->NodePosY = static_cast<int32>(Row * SpacingY);

		++Col;
		if (Col >= Cols)
		{
			Col = 0;
			++Row;
		}
	}

	Graph->NotifyGraphChanged();
	FBlueprintEditorUtils::MarkBlueprintAsModified(BP);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
	return FEpicUnrealMCPCommonUtils::CreateSuccessResponse(Result);
}

TSharedPtr<FJsonObject> FNodeLayoutManager::ArrangeNodesLinear(const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid parameters"));
	}

	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name'"));
	}

	UBlueprint* BP = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!BP)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	FString FunctionName;
	Params->TryGetStringField(TEXT("function_name"), FunctionName);

	UEdGraph* Graph = ResolveGraph(BP, FunctionName);
	if (!Graph)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
	}

	double StartX = 0.0, StartY = 0.0, SpacingX = 300.0;
	Params->TryGetNumberField(TEXT("start_x"), StartX);
	Params->TryGetNumberField(TEXT("start_y"), StartY);
	Params->TryGetNumberField(TEXT("spacing_x"), SpacingX);

	int32 Index = 0;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (!Node)
		{
			continue;
		}

		Node->NodePosX = static_cast<int32>(StartX + Index * SpacingX);
		Node->NodePosY = static_cast<int32>(StartY);
		++Index;
	}

	Graph->NotifyGraphChanged();
	FBlueprintEditorUtils::MarkBlueprintAsModified(BP);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetNumberField(TEXT("node_count"), Index);
	return FEpicUnrealMCPCommonUtils::CreateSuccessResponse(Result);
}

TSharedPtr<FJsonObject> FNodeLayoutManager::SetNodePosition(const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid parameters"));
	}

	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name'"));
	}

	FString NodeId;
	if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'node_id'"));
	}

	double PosX = 0.0, PosY = 0.0;
	if (!Params->TryGetNumberField(TEXT("pos_x"), PosX) || !Params->TryGetNumberField(TEXT("pos_y"), PosY))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pos_x' or 'pos_y'"));
	}

	UBlueprint* BP = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!BP)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	FString FunctionName;
	Params->TryGetStringField(TEXT("function_name"), FunctionName);

	UEdGraph* Graph = ResolveGraph(BP, FunctionName);
	if (!Graph)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
	}

	FGuid TargetGuid;
	FGuid::Parse(NodeId, TargetGuid);

	UEdGraphNode* TargetNode = nullptr;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Node && (Node->NodeGuid == TargetGuid || Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString().Equals(NodeId, ESearchCase::IgnoreCase)))
		{
			TargetNode = Node;
			break;
		}
	}

	if (!TargetNode)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Node not found: %s"), *NodeId));
	}

	TargetNode->NodePosX = static_cast<int32>(PosX);
	TargetNode->NodePosY = static_cast<int32>(PosY);

	Graph->NotifyGraphChanged();
	FBlueprintEditorUtils::MarkBlueprintAsModified(BP);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("node_id"), NodeId);
	Result->SetNumberField(TEXT("pos_x"), PosX);
	Result->SetNumberField(TEXT("pos_y"), PosY);
	return FEpicUnrealMCPCommonUtils::CreateSuccessResponse(Result);
}
