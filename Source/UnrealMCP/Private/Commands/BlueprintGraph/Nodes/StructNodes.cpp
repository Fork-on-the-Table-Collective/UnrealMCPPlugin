#include "Commands/BlueprintGraph/Nodes/StructNodes.h"
#include "Commands/BlueprintGraph/Nodes/NodeCreatorUtils.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_BreakStruct.h"
#include "Json.h"
#include "UObject/UObjectIterator.h"
#include "UObject/Package.h"

UScriptStruct* FStructNodeCreator::FindStructByName(const FString& StructName)
{
	// First try as exact object path
	UScriptStruct* Found = FindObject<UScriptStruct>(nullptr, *StructName, EFindObjectFlags::ExactClass);
	if (Found)
	{
		return Found;
	}

	// Fall back to iterating all script structs and matching short name
	for (TObjectIterator<UScriptStruct> It; It; ++It)
	{
		if (It->GetName().Equals(StructName, ESearchCase::IgnoreCase))
		{
			return *It;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("StructNodes: Struct type '%s' not found"), *StructName);
	return nullptr;
}

UK2Node* FStructNodeCreator::CreateMakeStructNode(UEdGraph* Graph, const TSharedPtr<FJsonObject>& Params)
{
	if (!Graph || !Params.IsValid())
	{
		return nullptr;
	}

	FString StructName;
	if (!Params->TryGetStringField(TEXT("struct_type"), StructName))
	{
		UE_LOG(LogTemp, Warning, TEXT("StructNodes::CreateMakeStructNode: Missing 'struct_type' param"));
		return nullptr;
	}

	UScriptStruct* TargetStruct = FindStructByName(StructName);
	if (!TargetStruct)
	{
		return nullptr;
	}

	UK2Node_MakeStruct* MakeNode = NewObject<UK2Node_MakeStruct>(Graph);
	if (!MakeNode)
	{
		return nullptr;
	}

	MakeNode->StructType = TargetStruct;

	double PosX = 0.0, PosY = 0.0;
	FNodeCreatorUtils::ExtractNodePosition(Params, PosX, PosY);
	MakeNode->NodePosX = static_cast<int32>(PosX);
	MakeNode->NodePosY = static_cast<int32>(PosY);

	Graph->AddNode(MakeNode, false, false);
	MakeNode->CreateNewGuid();
	MakeNode->PostPlacedNewNode();
	FNodeCreatorUtils::InitializeK2Node(MakeNode, Graph);

	return MakeNode;
}

UK2Node* FStructNodeCreator::CreateBreakStructNode(UEdGraph* Graph, const TSharedPtr<FJsonObject>& Params)
{
	if (!Graph || !Params.IsValid())
	{
		return nullptr;
	}

	FString StructName;
	if (!Params->TryGetStringField(TEXT("struct_type"), StructName))
	{
		UE_LOG(LogTemp, Warning, TEXT("StructNodes::CreateBreakStructNode: Missing 'struct_type' param"));
		return nullptr;
	}

	UScriptStruct* TargetStruct = FindStructByName(StructName);
	if (!TargetStruct)
	{
		return nullptr;
	}

	UK2Node_BreakStruct* BreakNode = NewObject<UK2Node_BreakStruct>(Graph);
	if (!BreakNode)
	{
		return nullptr;
	}

	BreakNode->StructType = TargetStruct;

	double PosX = 0.0, PosY = 0.0;
	FNodeCreatorUtils::ExtractNodePosition(Params, PosX, PosY);
	BreakNode->NodePosX = static_cast<int32>(PosX);
	BreakNode->NodePosY = static_cast<int32>(PosY);

	Graph->AddNode(BreakNode, false, false);
	BreakNode->CreateNewGuid();
	BreakNode->PostPlacedNewNode();
	FNodeCreatorUtils::InitializeK2Node(BreakNode, Graph);

	return BreakNode;
}
