#include "Commands/BlueprintGraph/CustomEventManager.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CustomEvent.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"

namespace
{
	static FEdGraphPinType PinTypeFromString(const FString& TypeString)
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
			PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
			PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
		}

		return PinType;
	}

	/** Find a custom event node in the event graph by event name. */
	static UK2Node_CustomEvent* FindCustomEventNode(UEdGraph* Graph, const FString& EventName)
	{
		if (!Graph)
		{
			return nullptr;
		}

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UK2Node_CustomEvent* CustomEvent = Cast<UK2Node_CustomEvent>(Node);
			if (CustomEvent && CustomEvent->CustomFunctionName.ToString().Equals(EventName, ESearchCase::IgnoreCase))
			{
				return CustomEvent;
			}
		}

		return nullptr;
	}
}

TSharedPtr<FJsonObject> FCustomEventManager::AddCustomEvent(const TSharedPtr<FJsonObject>& Params)
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

	FString EventName;
	if (!Params->TryGetStringField(TEXT("event_name"), EventName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'event_name'"));
	}

	UBlueprint* BP = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!BP)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	if (BP->UbergraphPages.Num() == 0)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint has no event graph"));
	}

	UEdGraph* Graph = BP->UbergraphPages[0];

	// Prevent duplicates
	if (FindCustomEventNode(Graph, EventName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Custom event '%s' already exists"), *EventName));
	}

	double PosX = 0.0, PosY = 0.0;
	Params->TryGetNumberField(TEXT("pos_x"), PosX);
	Params->TryGetNumberField(TEXT("pos_y"), PosY);

	UK2Node_CustomEvent* CustomEventNode = UK2Node_CustomEvent::CreateFromFunction(
		FVector2D(static_cast<float>(PosX), static_cast<float>(PosY)),
		Graph,
		EventName,
		nullptr,   // no delegate/function to copy signature from
		false      // don't auto-select
	);

	if (!CustomEventNode)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to create custom event: %s"), *EventName));
	}

	Graph->NotifyGraphChanged();
	FBlueprintEditorUtils::MarkBlueprintAsModified(BP);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("node_id"), CustomEventNode->NodeGuid.ToString());
	Result->SetStringField(TEXT("event_name"), EventName);
	return FEpicUnrealMCPCommonUtils::CreateSuccessResponse(Result);
}

TSharedPtr<FJsonObject> FCustomEventManager::AddEventParameter(const TSharedPtr<FJsonObject>& Params)
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

	FString EventName;
	if (!Params->TryGetStringField(TEXT("event_name"), EventName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'event_name'"));
	}

	FString ParameterName;
	if (!Params->TryGetStringField(TEXT("parameter_name"), ParameterName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'parameter_name'"));
	}

	FString ParameterType;
	if (!Params->TryGetStringField(TEXT("parameter_type"), ParameterType))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'parameter_type'"));
	}

	UBlueprint* BP = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!BP)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	if (BP->UbergraphPages.Num() == 0)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint has no event graph"));
	}

	UEdGraph* Graph = BP->UbergraphPages[0];
	UK2Node_CustomEvent* CustomEventNode = FindCustomEventNode(Graph, EventName);
	if (!CustomEventNode)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Custom event not found: %s"), *EventName));
	}

	FEdGraphPinType PinType = PinTypeFromString(ParameterType);

	// Custom events receive data via output pins (they emit the parameter values downstream)
	UEdGraphPin* NewPin = CustomEventNode->CreateUserDefinedPin(
		FName(*ParameterName),
		PinType,
		EGPD_Output,
		true /* use unique name if conflict */
	);

	if (!NewPin)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to add parameter '%s' to event '%s'"), *ParameterName, *EventName));
	}

	Graph->NotifyGraphChanged();
	FBlueprintEditorUtils::MarkBlueprintAsModified(BP);

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("event_name"), EventName);
	Result->SetStringField(TEXT("parameter_name"), NewPin->PinName.ToString());
	Result->SetStringField(TEXT("parameter_type"), ParameterType);
	return FEpicUnrealMCPCommonUtils::CreateSuccessResponse(Result);
}
