#include "Commands/BlueprintGraph/LocalVariableManager.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_FunctionEntry.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"

namespace
{
	static FEdGraphPinType LocalVar_PinTypeFromString(const FString& TypeString)
	{
		FEdGraphPinType PinType;

		if (TypeString.Equals(TEXT("bool"), ESearchCase::IgnoreCase))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
		}
		else if (TypeString.Equals(TEXT("int"), ESearchCase::IgnoreCase))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
		}
		else if (TypeString.Equals(TEXT("float"), ESearchCase::IgnoreCase))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
			PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
		}
		else if (TypeString.Equals(TEXT("string"), ESearchCase::IgnoreCase))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_String;
		}
		else if (TypeString.Equals(TEXT("vector"), ESearchCase::IgnoreCase))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
		}
		else if (TypeString.Equals(TEXT("rotator"), ESearchCase::IgnoreCase))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			PinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
		}
		else if (TypeString.Equals(TEXT("object"), ESearchCase::IgnoreCase))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
			PinType.PinSubCategoryObject = UObject::StaticClass();
		}
		else
		{
			// Default: float
			PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
			PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
		}

		return PinType;
	}

	/** Find the FunctionEntry node in a function graph; returns nullptr if not found. */
	static UK2Node_FunctionEntry* FindFunctionEntry(UEdGraph* FunctionGraph)
	{
		if (!FunctionGraph)
		{
			return nullptr;
		}

		for (UEdGraphNode* Node : FunctionGraph->Nodes)
		{
			if (UK2Node_FunctionEntry* Entry = Cast<UK2Node_FunctionEntry>(Node))
			{
				return Entry;
			}
		}

		return nullptr;
	}

	/** Resolve a function graph by name within a Blueprint. */
	static UEdGraph* FindFunctionGraph(UBlueprint* BP, const FString& FunctionName)
	{
		if (!BP)
		{
			return nullptr;
		}

		for (UEdGraph* Graph : BP->FunctionGraphs)
		{
			if (Graph && Graph->GetName().Equals(FunctionName, ESearchCase::IgnoreCase))
			{
				return Graph;
			}
		}

		// Partial match fallback
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

TSharedPtr<FJsonObject> FLocalVariableManager::AddLocalVariable(const TSharedPtr<FJsonObject>& Params)
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

	FString FunctionName;
	if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name'"));
	}

	FString VariableName;
	if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name'"));
	}

	FString VariableType;
	if (!Params->TryGetStringField(TEXT("variable_type"), VariableType))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_type'"));
	}

	UBlueprint* BP = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!BP)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* Graph = FindFunctionGraph(BP, FunctionName);
	if (!Graph)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Function not found: %s"), *FunctionName));
	}

	UK2Node_FunctionEntry* EntryNode = FindFunctionEntry(Graph);
	if (!EntryNode)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Function entry node not found"));
	}

	FBPVariableDescription NewVar;
	NewVar.VarName = FName(*VariableName);
	NewVar.VarType = LocalVar_PinTypeFromString(VariableType);
	NewVar.FriendlyName = VariableName;
	NewVar.Category = FText::FromString(TEXT("Local"));

	// Apply optional default value
	FString DefaultValue;
	if (Params->TryGetStringField(TEXT("default_value"), DefaultValue))
	{
		NewVar.DefaultValue = DefaultValue;
	}

	EntryNode->LocalVariables.Add(NewVar);
	EntryNode->ReconstructNode();

	FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
	FKismetEditorUtilities::CompileBlueprint(BP);

	TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();
	VarObj->SetStringField(TEXT("name"), VariableName);
	VarObj->SetStringField(TEXT("type"), VariableType);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetObjectField(TEXT("local_variable"), VarObj);
	return FEpicUnrealMCPCommonUtils::CreateSuccessResponse(Result);
}

TSharedPtr<FJsonObject> FLocalVariableManager::GetLocalVariables(const TSharedPtr<FJsonObject>& Params)
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

	FString FunctionName;
	if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name'"));
	}

	UBlueprint* BP = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!BP)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* Graph = FindFunctionGraph(BP, FunctionName);
	if (!Graph)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Function not found: %s"), *FunctionName));
	}

	UK2Node_FunctionEntry* EntryNode = FindFunctionEntry(Graph);
	if (!EntryNode)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Function entry node not found"));
	}

	TArray<TSharedPtr<FJsonValue>> VarsArray;
	for (const FBPVariableDescription& Var : EntryNode->LocalVariables)
	{
		TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();
		VarObj->SetStringField(TEXT("name"), Var.VarName.ToString());
		VarObj->SetStringField(TEXT("type"), Var.VarType.PinCategory.ToString());
		if (!Var.DefaultValue.IsEmpty())
		{
			VarObj->SetStringField(TEXT("default_value"), Var.DefaultValue);
		}
		VarsArray.Add(MakeShared<FJsonValueObject>(VarObj));
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetArrayField(TEXT("local_variables"), VarsArray);
	return FEpicUnrealMCPCommonUtils::CreateSuccessResponse(Result);
}

TSharedPtr<FJsonObject> FLocalVariableManager::RemoveLocalVariable(const TSharedPtr<FJsonObject>& Params)
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

	FString FunctionName;
	if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name'"));
	}

	FString VariableName;
	if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name'"));
	}

	UBlueprint* BP = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!BP)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* Graph = FindFunctionGraph(BP, FunctionName);
	if (!Graph)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Function not found: %s"), *FunctionName));
	}

	UK2Node_FunctionEntry* EntryNode = FindFunctionEntry(Graph);
	if (!EntryNode)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Function entry node not found"));
	}

	const FName VarName(*VariableName);
	const int32 RemovedCount = EntryNode->LocalVariables.RemoveAll(
		[&VarName](const FBPVariableDescription& Desc)
		{
			return Desc.VarName == VarName;
		});

	if (RemovedCount == 0)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Local variable not found: %s"), *VariableName));
	}

	EntryNode->ReconstructNode();
	FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
	FKismetEditorUtilities::CompileBlueprint(BP);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("removed_variable"), VariableName);
	return FEpicUnrealMCPCommonUtils::CreateSuccessResponse(Result);
}
