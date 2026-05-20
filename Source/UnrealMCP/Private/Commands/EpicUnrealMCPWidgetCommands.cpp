// UMG/Widget Designer MCP Commands Implementation
#include "Commands/EpicUnrealMCPWidgetCommands.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditorUtils.h"
#include "Components/Widget.h"
#include "Components/PanelWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelSlot.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Dom/JsonObject.h"
#include "JsonObjectConverter.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "PackageTools.h"
#include "Styling/SlateColor.h"
#include "Layout/Margin.h"
#include "UObject/UnrealType.h"

FEpicUnrealMCPWidgetCommands::FEpicUnrealMCPWidgetCommands()
{
}

TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	// Widget Blueprint management
	if (CommandType == TEXT("create_widget_blueprint"))
	{
		return HandleCreateWidgetBlueprint(Params);
	}
	else if (CommandType == TEXT("delete_widget_blueprint"))
	{
		return HandleDeleteWidgetBlueprint(Params);
	}
	else if (CommandType == TEXT("get_widget_blueprint_info"))
	{
		return HandleGetWidgetBlueprintInfo(Params);
	}
	else if (CommandType == TEXT("list_all_widgets"))
	{
		return HandleListAllWidgets(Params);
	}
	// Widget tree operations
	else if (CommandType == TEXT("get_widget_tree"))
	{
		return HandleGetWidgetTree(Params);
	}
	else if (CommandType == TEXT("add_widget_to_tree"))
	{
		return HandleAddWidgetToTree(Params);
	}
	else if (CommandType == TEXT("remove_widget_from_tree"))
	{
		return HandleRemoveWidgetFromTree(Params);
	}
	else if (CommandType == TEXT("reparent_widget"))
	{
		return HandleReparentWidget(Params);
	}
	// Widget properties
	else if (CommandType == TEXT("get_widget_properties"))
	{
		return HandleGetWidgetProperties(Params);
	}
	else if (CommandType == TEXT("set_widget_property"))
	{
		return HandleSetWidgetProperty(Params);
	}
	else if (CommandType == TEXT("set_widget_transform"))
	{
		return HandleSetWidgetTransform(Params);
	}
	else if (CommandType == TEXT("set_widget_anchors"))
	{
		return HandleSetWidgetAnchors(Params);
	}
	// Slot properties
	else if (CommandType == TEXT("get_slot_properties"))
	{
		return HandleGetSlotProperties(Params);
	}
	else if (CommandType == TEXT("set_slot_property"))
	{
		return HandleSetSlotProperty(Params);
	}

	return CreateErrorResponse(TEXT("Unknown widget command: ") + CommandType);
}

// Create a new Widget Blueprint
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleCreateWidgetBlueprint(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetPath = Params->GetStringField(TEXT("widget_path"));
	FString ParentClass = Params->HasField(TEXT("parent_class")) ? Params->GetStringField(TEXT("parent_class")) : TEXT("/Script/UMG.UserWidget");

	// Check if widget already exists
	if (UEditorAssetLibrary::DoesAssetExist(WidgetPath))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Widget Blueprint already exists: %s"), *WidgetPath));
	}

	// Create the package path
	FString PackageName, AssetName;
	WidgetPath.Split(TEXT("/"), &PackageName, &AssetName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	if (PackageName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Invalid widget path"));
	}

	// Use FKismetEditorUtilities to create a Widget Blueprint similar to regular Blueprints
	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		return CreateErrorResponse(TEXT("Failed to create package"));
	}

	UClass* WidgetClass = UUserWidget::StaticClass();
	
	// Create the Widget Blueprint using FKismetEditorUtilities
	UWidgetBlueprint* NewWidgetBP = Cast<UWidgetBlueprint>(
		FKismetEditorUtilities::CreateBlueprint(
			WidgetClass,
			Package,
			*AssetName,
			BPTYPE_Normal,
			UWidgetBlueprint::StaticClass(),
			UBlueprintGeneratedClass::StaticClass(),
			NAME_None
		)
	);

	if (!NewWidgetBP)
	{
		return CreateErrorResponse(TEXT("Failed to create Widget Blueprint"));
	}

	// Add a root Canvas Panel by default
	if (!NewWidgetBP->WidgetTree)
	{
		NewWidgetBP->WidgetTree = NewObject<UWidgetTree>(NewWidgetBP, NAME_None, RF_Transactional);
	}
	
	UWidgetTree* WidgetTree = NewWidgetBP->WidgetTree;
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;
	}

	// Compile the blueprint
	FKismetEditorUtilities::CompileBlueprint(NewWidgetBP);

	// Mark package as dirty and save
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewWidgetBP);
	
	UEditorAssetLibrary::SaveAsset(WidgetPath, false);

	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("status"), TEXT("success"));
	Response->SetStringField(TEXT("widget_path"), WidgetPath);
	Response->SetStringField(TEXT("message"), TEXT("Widget Blueprint created successfully"));
	return Response;
}

// Delete a Widget Blueprint
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleDeleteWidgetBlueprint(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetPath = Params->GetStringField(TEXT("widget_path"));

	if (!UEditorAssetLibrary::DoesAssetExist(WidgetPath))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Widget Blueprint does not exist: %s"), *WidgetPath));
	}

	bool bSuccess = UEditorAssetLibrary::DeleteAsset(WidgetPath);
	
	if (bSuccess)
	{
		return CreateSuccessResponse(TEXT("Widget Blueprint deleted successfully"));
	}
	else
	{
		return CreateErrorResponse(TEXT("Failed to delete Widget Blueprint"));
	}
}

// Get Widget Blueprint information
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleGetWidgetBlueprintInfo(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetPath = Params->GetStringField(TEXT("widget_path"));
	
	UWidgetBlueprint* WidgetBP = LoadWidgetBlueprint(WidgetPath);
	if (!WidgetBP)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to load Widget Blueprint: %s"), *WidgetPath));
	}

	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("status"), TEXT("success"));
	Response->SetStringField(TEXT("widget_path"), WidgetPath);
	Response->SetStringField(TEXT("widget_name"), WidgetBP->GetName());
	
	if (WidgetBP->ParentClass)
	{
		Response->SetStringField(TEXT("parent_class"), WidgetBP->ParentClass->GetPathName());
	}

	// Get root widget info
	if (WidgetBP->WidgetTree && WidgetBP->WidgetTree->RootWidget)
	{
		UWidget* RootWidget = WidgetBP->WidgetTree->RootWidget;
		Response->SetStringField(TEXT("root_widget_name"), RootWidget->GetName());
		Response->SetStringField(TEXT("root_widget_class"), RootWidget->GetClass()->GetName());
	}

	return Response;
}

// List all Widget Blueprints in the project
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleListAllWidgets(const TSharedPtr<FJsonObject>& Params)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetDataList;
	
	FARFilter Filter;
	Filter.ClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;

	AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

	TArray<TSharedPtr<FJsonValue>> WidgetsArray;
	for (const FAssetData& AssetData : AssetDataList)
	{
		TSharedPtr<FJsonObject> WidgetObj = MakeShared<FJsonObject>();
		WidgetObj->SetStringField(TEXT("widget_path"), AssetData.GetObjectPathString());
		WidgetObj->SetStringField(TEXT("widget_name"), AssetData.AssetName.ToString());
		WidgetObj->SetStringField(TEXT("package_path"), AssetData.PackagePath.ToString());
		WidgetsArray.Add(MakeShared<FJsonValueObject>(WidgetObj));
	}

	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("status"), TEXT("success"));
	Response->SetArrayField(TEXT("widgets"), WidgetsArray);
	Response->SetNumberField(TEXT("count"), WidgetsArray.Num());
	return Response;
}

// Get the widget tree hierarchy
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleGetWidgetTree(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetPath = Params->GetStringField(TEXT("widget_path"));
	
	UWidgetBlueprint* WidgetBP = LoadWidgetBlueprint(WidgetPath);
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{
		return CreateErrorResponse(TEXT("Failed to load Widget Blueprint or widget tree"));
	}

	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("status"), TEXT("success"));

	// Build widget tree recursively
	TFunction<TSharedPtr<FJsonObject>(UWidget*)> BuildWidgetTreeJson = [&](UWidget* Widget) -> TSharedPtr<FJsonObject>
	{
		if (!Widget)
		{
			return nullptr;
		}

		TSharedPtr<FJsonObject> WidgetJson = MakeShared<FJsonObject>();
		WidgetJson->SetStringField(TEXT("name"), Widget->GetName());
		WidgetJson->SetStringField(TEXT("class"), Widget->GetClass()->GetName());
		WidgetJson->SetBoolField(TEXT("is_visible"), Widget->GetVisibility() == ESlateVisibility::Visible);

		// If it's a panel widget, get children
		UPanelWidget* PanelWidget = Cast<UPanelWidget>(Widget);
		if (PanelWidget)
		{
			TArray<TSharedPtr<FJsonValue>> ChildrenArray;
			for (int32 i = 0; i < PanelWidget->GetChildrenCount(); ++i)
			{
				UWidget* Child = PanelWidget->GetChildAt(i);
				TSharedPtr<FJsonObject> ChildJson = BuildWidgetTreeJson(Child);
				if (ChildJson.IsValid())
				{
					ChildrenArray.Add(MakeShared<FJsonValueObject>(ChildJson));
				}
			}
			WidgetJson->SetArrayField(TEXT("children"), ChildrenArray);
		}

		return WidgetJson;
	};

	if (WidgetBP->WidgetTree->RootWidget)
	{
		TSharedPtr<FJsonObject> RootJson = BuildWidgetTreeJson(WidgetBP->WidgetTree->RootWidget);
		Response->SetObjectField(TEXT("widget_tree"), RootJson);
	}

	return Response;
}

// Add a widget to the widget tree
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleAddWidgetToTree(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetPath = Params->GetStringField(TEXT("widget_path"));
	FString WidgetClass = Params->GetStringField(TEXT("widget_class"));
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	FString ParentName = Params->HasField(TEXT("parent_name")) ? Params->GetStringField(TEXT("parent_name")) : TEXT("");

	UWidgetBlueprint* WidgetBP = LoadWidgetBlueprint(WidgetPath);
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{
		return CreateErrorResponse(TEXT("Failed to load Widget Blueprint"));
	}

	// Find the widget class
	UClass* WidgetUClass = FindObject<UClass>(nullptr, *WidgetClass);
	if (!WidgetUClass)
	{
		// Try with "U" prefix for UMG widgets
		FString UClassName = TEXT("U") + WidgetClass;
		WidgetUClass = FindObject<UClass>(nullptr, *UClassName);
	}

	if (!WidgetUClass)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Widget class not found: %s"), *WidgetClass));
	}

	// Create the new widget
	UWidget* NewWidget = WidgetBP->WidgetTree->ConstructWidget<UWidget>(WidgetUClass, *WidgetName);
	if (!NewWidget)
	{
		return CreateErrorResponse(TEXT("Failed to create widget"));
	}

	// Find parent widget
	UPanelWidget* ParentWidget = nullptr;
	if (!ParentName.IsEmpty())
	{
		UWidget* Parent = FindWidgetInTree(WidgetBP, ParentName);
		ParentWidget = Cast<UPanelWidget>(Parent);
		if (!ParentWidget)
		{
			return CreateErrorResponse(FString::Printf(TEXT("Parent widget not found or is not a panel: %s"), *ParentName));
		}
	}
	else
	{
		// Use root widget as parent
		ParentWidget = Cast<UPanelWidget>(WidgetBP->WidgetTree->RootWidget);
	}

	if (!ParentWidget)
	{
		return CreateErrorResponse(TEXT("No valid parent widget found"));
	}

	// Add the widget to the parent
	UPanelSlot* Slot = ParentWidget->AddChild(NewWidget);
	if (!Slot)
	{
		return CreateErrorResponse(TEXT("Failed to add widget to parent"));
	}

	// Compile the blueprint
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("status"), TEXT("success"));
	Response->SetStringField(TEXT("widget_name"), WidgetName);
	Response->SetStringField(TEXT("message"), TEXT("Widget added successfully"));
	return Response;
}

// Remove a widget from the widget tree
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleRemoveWidgetFromTree(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetPath = Params->GetStringField(TEXT("widget_path"));
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));

	UWidgetBlueprint* WidgetBP = LoadWidgetBlueprint(WidgetPath);
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{
		return CreateErrorResponse(TEXT("Failed to load Widget Blueprint"));
	}

	UWidget* WidgetToRemove = FindWidgetInTree(WidgetBP, WidgetName);
	if (!WidgetToRemove)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Widget not found: %s"), *WidgetName));
	}

	// Can't remove root widget
	if (WidgetToRemove == WidgetBP->WidgetTree->RootWidget)
	{
		return CreateErrorResponse(TEXT("Cannot remove root widget"));
	}

	// Remove from parent
	UWidget* Parent = WidgetToRemove->GetParent();
	UPanelWidget* ParentPanel = Cast<UPanelWidget>(Parent);
	if (ParentPanel)
	{
		ParentPanel->RemoveChild(WidgetToRemove);
	}

	// Compile the blueprint
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	return CreateSuccessResponse(TEXT("Widget removed successfully"));
}

// Reparent a widget (move it to a different parent)
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleReparentWidget(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetPath = Params->GetStringField(TEXT("widget_path"));
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	FString NewParentName = Params->GetStringField(TEXT("new_parent_name"));

	UWidgetBlueprint* WidgetBP = LoadWidgetBlueprint(WidgetPath);
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{
		return CreateErrorResponse(TEXT("Failed to load Widget Blueprint"));
	}

	UWidget* Widget = FindWidgetInTree(WidgetBP, WidgetName);
	if (!Widget)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Widget not found: %s"), *WidgetName));
	}

	UWidget* NewParent = FindWidgetInTree(WidgetBP, NewParentName);
	UPanelWidget* NewParentPanel = Cast<UPanelWidget>(NewParent);
	if (!NewParentPanel)
	{
		return CreateErrorResponse(FString::Printf(TEXT("New parent not found or is not a panel: %s"), *NewParentName));
	}

	// Remove from current parent
	UWidget* CurrentParent = Widget->GetParent();
	UPanelWidget* CurrentParentPanel = Cast<UPanelWidget>(CurrentParent);
	if (CurrentParentPanel)
	{
		CurrentParentPanel->RemoveChild(Widget);
	}

	// Add to new parent
	NewParentPanel->AddChild(Widget);

	// Compile the blueprint
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);

	return CreateSuccessResponse(TEXT("Widget reparented successfully"));
}

// Get widget properties
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleGetWidgetProperties(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetPath = Params->GetStringField(TEXT("widget_path"));
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));

	UWidgetBlueprint* WidgetBP = LoadWidgetBlueprint(WidgetPath);
	if (!WidgetBP)
	{
		return CreateErrorResponse(TEXT("Failed to load Widget Blueprint"));
	}

	UWidget* Widget = FindWidgetInTree(WidgetBP, WidgetName);
	if (!Widget)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Widget not found: %s"), *WidgetName));
	}

	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("status"), TEXT("success"));
	Response->SetObjectField(TEXT("properties"), WidgetToJson(Widget));
	
	return Response;
}

// Set a widget property
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleSetWidgetProperty(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetPath = Params->GetStringField(TEXT("widget_path"));
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	FString PropertyName = Params->GetStringField(TEXT("property_name"));

	if (!Params->HasField(TEXT("value")))
	{
		return CreateErrorResponse(TEXT("Missing 'value' field"));
	}

	UWidgetBlueprint* WidgetBP = LoadWidgetBlueprint(WidgetPath);
	if (!WidgetBP)
	{
		return CreateErrorResponse(TEXT("Failed to load Widget Blueprint"));
	}

	UWidget* Widget = FindWidgetInTree(WidgetBP, WidgetName);
	if (!Widget)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Widget not found: %s"), *WidgetName));
	}

	// Find the property using reflection
	FProperty* Property = FindFProperty<FProperty>(Widget->GetClass(), *PropertyName);
	if (!Property)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Property not found: %s"), *PropertyName));
	}

	void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Widget);
	bool bSuccess = false;

	// Set property value based on type
	if (FNumericProperty* NumericProp = CastField<FNumericProperty>(Property))
	{
		if (Params->HasTypedField<EJson::Number>(TEXT("value")))
		{
			double Value = Params->GetNumberField(TEXT("value"));
			if (NumericProp->IsFloatingPoint())
			{
				NumericProp->SetFloatingPointPropertyValue(ValuePtr, Value);
				bSuccess = true;
			}
			else if (NumericProp->IsInteger())
			{
				NumericProp->SetIntPropertyValue(ValuePtr, static_cast<int64>(Value));
				bSuccess = true;
			}
		}
	}
	else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
	{
		if (Params->HasTypedField<EJson::Boolean>(TEXT("value")))
		{
			bool Value = Params->GetBoolField(TEXT("value"));
			BoolProp->SetPropertyValue(ValuePtr, Value);
			bSuccess = true;
		}
	}
	else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
	{
		if (Params->HasTypedField<EJson::String>(TEXT("value")))
		{
			FString Value = Params->GetStringField(TEXT("value"));
			StrProp->SetPropertyValue(ValuePtr, Value);
			bSuccess = true;
		}
	}
	else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
	{
		if (Params->HasTypedField<EJson::String>(TEXT("value")))
		{
			FString Value = Params->GetStringField(TEXT("value"));
			NameProp->SetPropertyValue(ValuePtr, FName(*Value));
			bSuccess = true;
		}
	}
	else if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
	{
		if (Params->HasTypedField<EJson::String>(TEXT("value")))
		{
			FString Value = Params->GetStringField(TEXT("value"));
			TextProp->SetPropertyValue(ValuePtr, FText::FromString(Value));
			bSuccess = true;
		}
	}
	else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
	{
		if (Params->HasTypedField<EJson::String>(TEXT("value")))
		{
			FString Value = Params->GetStringField(TEXT("value"));
			UEnum* EnumType = EnumProp->GetEnum();
			int64 EnumValue = EnumType->GetValueByNameString(Value);
			if (EnumValue != INDEX_NONE)
			{
				FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
				UnderlyingProp->SetIntPropertyValue(ValuePtr, EnumValue);
				bSuccess = true;
			}
		}
		else if (Params->HasTypedField<EJson::Number>(TEXT("value")))
		{
			int64 EnumValue = static_cast<int64>(Params->GetNumberField(TEXT("value")));
			FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
			UnderlyingProp->SetIntPropertyValue(ValuePtr, EnumValue);
			bSuccess = true;
		}
	}
	else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
	{
		if (Params->HasTypedField<EJson::Object>(TEXT("value")))
		{
			const TSharedPtr<FJsonObject>* ValueObj;
			if (Params->TryGetObjectField(TEXT("value"), ValueObj))
			{
				UScriptStruct* Struct = StructProp->Struct;
				if (Struct->GetName() == TEXT("Vector2D"))
				{
					FVector2D* Vec = static_cast<FVector2D*>(ValuePtr);
					if ((*ValueObj)->HasField(TEXT("x")))
						Vec->X = (*ValueObj)->GetNumberField(TEXT("x"));
					if ((*ValueObj)->HasField(TEXT("y")))
						Vec->Y = (*ValueObj)->GetNumberField(TEXT("y"));
					bSuccess = true;
				}
				else if (Struct->GetName() == TEXT("Vector"))
				{
					FVector* Vec = static_cast<FVector*>(ValuePtr);
					if ((*ValueObj)->HasField(TEXT("x")))
						Vec->X = (*ValueObj)->GetNumberField(TEXT("x"));
					if ((*ValueObj)->HasField(TEXT("y")))
						Vec->Y = (*ValueObj)->GetNumberField(TEXT("y"));
					if ((*ValueObj)->HasField(TEXT("z")))
						Vec->Z = (*ValueObj)->GetNumberField(TEXT("z"));
					bSuccess = true;
				}
				else if (Struct->GetName() == TEXT("LinearColor"))
				{
					FLinearColor* Color = static_cast<FLinearColor*>(ValuePtr);
					if ((*ValueObj)->HasField(TEXT("r")))
						Color->R = (*ValueObj)->GetNumberField(TEXT("r"));
					if ((*ValueObj)->HasField(TEXT("g")))
						Color->G = (*ValueObj)->GetNumberField(TEXT("g"));
					if ((*ValueObj)->HasField(TEXT("b")))
						Color->B = (*ValueObj)->GetNumberField(TEXT("b"));
					if ((*ValueObj)->HasField(TEXT("a")))
						Color->A = (*ValueObj)->GetNumberField(TEXT("a"));
					bSuccess = true;
				}
				else if (Struct->GetName() == TEXT("Margin"))
				{
					FMargin* Margin = static_cast<FMargin*>(ValuePtr);
					if ((*ValueObj)->HasField(TEXT("left")))
						Margin->Left = (*ValueObj)->GetNumberField(TEXT("left"));
					if ((*ValueObj)->HasField(TEXT("top")))
						Margin->Top = (*ValueObj)->GetNumberField(TEXT("top"));
					if ((*ValueObj)->HasField(TEXT("right")))
						Margin->Right = (*ValueObj)->GetNumberField(TEXT("right"));
					if ((*ValueObj)->HasField(TEXT("bottom")))
						Margin->Bottom = (*ValueObj)->GetNumberField(TEXT("bottom"));
					bSuccess = true;
				}
			}
		}
	}

	if (!bSuccess)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to set property '%s' - type mismatch or unsupported type"), *PropertyName));
	}

	// Mark modified and compile
	Widget->Modify();
	WidgetBP->Modify();
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);
	
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("status"), TEXT("success"));
	Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Property '%s' set successfully"), *PropertyName));
	return Response;
}

// Set widget transform (position, size)
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleSetWidgetTransform(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetPath = Params->GetStringField(TEXT("widget_path"));
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));

	UWidgetBlueprint* WidgetBP = LoadWidgetBlueprint(WidgetPath);
	if (!WidgetBP)
	{
		return CreateErrorResponse(TEXT("Failed to load Widget Blueprint"));
	}

	UWidget* Widget = FindWidgetInTree(WidgetBP, WidgetName);
	if (!Widget)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Widget not found: %s"), *WidgetName));
	}

	// Only canvas panel slots support position/size
	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot);
	if (!CanvasSlot)
	{
		return CreateErrorResponse(TEXT("Widget is not in a Canvas Panel"));
	}

	// Set position
	if (Params->HasField(TEXT("position")))
	{
		const TArray<TSharedPtr<FJsonValue>>* PosArray;
		if (Params->TryGetArrayField(TEXT("position"), PosArray) && PosArray->Num() == 2)
		{
			float X = (*PosArray)[0]->AsNumber();
			float Y = (*PosArray)[1]->AsNumber();
			CanvasSlot->SetPosition(FVector2D(X, Y));
		}
	}

	// Set size
	if (Params->HasField(TEXT("size")))
	{
		const TArray<TSharedPtr<FJsonValue>>* SizeArray;
		if (Params->TryGetArrayField(TEXT("size"), SizeArray) && SizeArray->Num() == 2)
		{
			float Width = (*SizeArray)[0]->AsNumber();
			float Height = (*SizeArray)[1]->AsNumber();
			CanvasSlot->SetSize(FVector2D(Width, Height));
		}
	}

	FKismetEditorUtilities::CompileBlueprint(WidgetBP);
	return CreateSuccessResponse(TEXT("Widget transform set successfully"));
}

// Set widget anchors
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleSetWidgetAnchors(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetPath = Params->GetStringField(TEXT("widget_path"));
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));

	UWidgetBlueprint* WidgetBP = LoadWidgetBlueprint(WidgetPath);
	if (!WidgetBP)
	{
		return CreateErrorResponse(TEXT("Failed to load Widget Blueprint"));
	}

	UWidget* Widget = FindWidgetInTree(WidgetBP, WidgetName);
	if (!Widget)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Widget not found: %s"), *WidgetName));
	}

	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot);
	if (!CanvasSlot)
	{
		return CreateErrorResponse(TEXT("Widget is not in a Canvas Panel"));
	}

	// Set anchors
	if (Params->HasField(TEXT("anchors")))
	{
		const TSharedPtr<FJsonObject>* AnchorsObj;
		if (Params->TryGetObjectField(TEXT("anchors"), AnchorsObj))
		{
			FAnchors Anchors;
			Anchors.Minimum.X = (*AnchorsObj)->GetNumberField(TEXT("min_x"));
			Anchors.Minimum.Y = (*AnchorsObj)->GetNumberField(TEXT("min_y"));
			Anchors.Maximum.X = (*AnchorsObj)->GetNumberField(TEXT("max_x"));
			Anchors.Maximum.Y = (*AnchorsObj)->GetNumberField(TEXT("max_y"));
			CanvasSlot->SetAnchors(Anchors);
		}
	}

	FKismetEditorUtilities::CompileBlueprint(WidgetBP);
	return CreateSuccessResponse(TEXT("Widget anchors set successfully"));
}

// Get slot properties
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleGetSlotProperties(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetPath = Params->GetStringField(TEXT("widget_path"));
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));

	UWidgetBlueprint* WidgetBP = LoadWidgetBlueprint(WidgetPath);
	if (!WidgetBP)
	{
		return CreateErrorResponse(TEXT("Failed to load Widget Blueprint"));
	}

	UWidget* Widget = FindWidgetInTree(WidgetBP, WidgetName);
	if (!Widget || !Widget->Slot)
	{
		return CreateErrorResponse(TEXT("Widget or slot not found"));
	}

	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("status"), TEXT("success"));
	Response->SetStringField(TEXT("slot_class"), Widget->Slot->GetClass()->GetName());

	// Add Canvas Panel Slot specific properties
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
	{
		TSharedPtr<FJsonObject> SlotProps = MakeShared<FJsonObject>();
		
		FVector2D Position = CanvasSlot->GetPosition();
		TArray<TSharedPtr<FJsonValue>> PosArray;
		PosArray.Add(MakeShared<FJsonValueNumber>(Position.X));
		PosArray.Add(MakeShared<FJsonValueNumber>(Position.Y));
		SlotProps->SetArrayField(TEXT("position"), PosArray);

		FVector2D Size = CanvasSlot->GetSize();
		TArray<TSharedPtr<FJsonValue>> SizeArray;
		SizeArray.Add(MakeShared<FJsonValueNumber>(Size.X));
		SizeArray.Add(MakeShared<FJsonValueNumber>(Size.Y));
		SlotProps->SetArrayField(TEXT("size"), SizeArray);

		FAnchors Anchors = CanvasSlot->GetAnchors();
		TSharedPtr<FJsonObject> AnchorsObj = MakeShared<FJsonObject>();
		AnchorsObj->SetNumberField(TEXT("min_x"), Anchors.Minimum.X);
		AnchorsObj->SetNumberField(TEXT("min_y"), Anchors.Minimum.Y);
		AnchorsObj->SetNumberField(TEXT("max_x"), Anchors.Maximum.X);
		AnchorsObj->SetNumberField(TEXT("max_y"), Anchors.Maximum.Y);
		SlotProps->SetObjectField(TEXT("anchors"), AnchorsObj);

		Response->SetObjectField(TEXT("properties"), SlotProps);
	}

	return Response;
}

// Set slot property
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleSetSlotProperty(const TSharedPtr<FJsonObject>& Params)
{
	FString WidgetPath = Params->GetStringField(TEXT("widget_path"));
	FString WidgetName = Params->GetStringField(TEXT("widget_name"));
	FString PropertyName = Params->GetStringField(TEXT("property_name"));

	if (!Params->HasField(TEXT("value")))
	{
		return CreateErrorResponse(TEXT("Missing 'value' field"));
	}

	UWidgetBlueprint* WidgetBP = LoadWidgetBlueprint(WidgetPath);
	if (!WidgetBP)
	{
		return CreateErrorResponse(TEXT("Failed to load Widget Blueprint"));
	}

	UWidget* Widget = FindWidgetInTree(WidgetBP, WidgetName);
	if (!Widget || !Widget->Slot)
	{
		return CreateErrorResponse(TEXT("Widget or slot not found"));
	}

	UPanelSlot* Slot = Widget->Slot;

	// Find the property using reflection
	FProperty* Property = FindFProperty<FProperty>(Slot->GetClass(), *PropertyName);
	if (!Property)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Slot property not found: %s"), *PropertyName));
	}

	void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Slot);
	bool bSuccess = false;

	// Set property value based on type (same logic as widget properties)
	if (FNumericProperty* NumericProp = CastField<FNumericProperty>(Property))
	{
		if (Params->HasTypedField<EJson::Number>(TEXT("value")))
		{
			double Value = Params->GetNumberField(TEXT("value"));
			if (NumericProp->IsFloatingPoint())
			{
				NumericProp->SetFloatingPointPropertyValue(ValuePtr, Value);
				bSuccess = true;
			}
			else if (NumericProp->IsInteger())
			{
				NumericProp->SetIntPropertyValue(ValuePtr, static_cast<int64>(Value));
				bSuccess = true;
			}
		}
	}
	else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
	{
		if (Params->HasTypedField<EJson::Boolean>(TEXT("value")))
		{
			bool Value = Params->GetBoolField(TEXT("value"));
			BoolProp->SetPropertyValue(ValuePtr, Value);
			bSuccess = true;
		}
	}
	else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
	{
		if (Params->HasTypedField<EJson::Object>(TEXT("value")))
		{
			const TSharedPtr<FJsonObject>* ValueObj;
			if (Params->TryGetObjectField(TEXT("value"), ValueObj))
			{
				UScriptStruct* Struct = StructProp->Struct;
				if (Struct->GetName() == TEXT("Vector2D"))
				{
					FVector2D* Vec = static_cast<FVector2D*>(ValuePtr);
					if ((*ValueObj)->HasField(TEXT("x")))
						Vec->X = (*ValueObj)->GetNumberField(TEXT("x"));
					if ((*ValueObj)->HasField(TEXT("y")))
						Vec->Y = (*ValueObj)->GetNumberField(TEXT("y"));
					bSuccess = true;
				}
				else if (Struct->GetName() == TEXT("Margin"))
				{
					FMargin* Margin = static_cast<FMargin*>(ValuePtr);
					if ((*ValueObj)->HasField(TEXT("left")))
						Margin->Left = (*ValueObj)->GetNumberField(TEXT("left"));
					if ((*ValueObj)->HasField(TEXT("top")))
						Margin->Top = (*ValueObj)->GetNumberField(TEXT("top"));
					if ((*ValueObj)->HasField(TEXT("right")))
						Margin->Right = (*ValueObj)->GetNumberField(TEXT("right"));
					if ((*ValueObj)->HasField(TEXT("bottom")))
						Margin->Bottom = (*ValueObj)->GetNumberField(TEXT("bottom"));
					bSuccess = true;
				}
				else if (Struct->GetName() == TEXT("Anchors"))
				{
					FAnchors* Anchors = static_cast<FAnchors*>(ValuePtr);
					if ((*ValueObj)->HasField(TEXT("min_x")))
						Anchors->Minimum.X = (*ValueObj)->GetNumberField(TEXT("min_x"));
					if ((*ValueObj)->HasField(TEXT("min_y")))
						Anchors->Minimum.Y = (*ValueObj)->GetNumberField(TEXT("min_y"));
					if ((*ValueObj)->HasField(TEXT("max_x")))
						Anchors->Maximum.X = (*ValueObj)->GetNumberField(TEXT("max_x"));
					if ((*ValueObj)->HasField(TEXT("max_y")))
						Anchors->Maximum.Y = (*ValueObj)->GetNumberField(TEXT("max_y"));
					bSuccess = true;
				}
			}
		}
	}

	if (!bSuccess)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to set slot property '%s' - type mismatch or unsupported type"), *PropertyName));
	}

	// Mark modified and compile
	Slot->Modify();
	Widget->Modify();
	WidgetBP->Modify();
	FKismetEditorUtilities::CompileBlueprint(WidgetBP);
	
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("status"), TEXT("success"));
	Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Slot property '%s' set successfully"), *PropertyName));
	return Response;
}

// Utility: Load Widget Blueprint
UWidgetBlueprint* FEpicUnrealMCPWidgetCommands::LoadWidgetBlueprint(const FString& WidgetPath)
{
	UObject* LoadedObject = UEditorAssetLibrary::LoadAsset(WidgetPath);
	return Cast<UWidgetBlueprint>(LoadedObject);
}

// Utility: Find widget in tree by name
UWidget* FEpicUnrealMCPWidgetCommands::FindWidgetInTree(UWidgetBlueprint* WidgetBP, const FString& WidgetName)
{
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{
		return nullptr;
	}

	TArray<UWidget*> AllWidgets;
	WidgetBP->WidgetTree->GetAllWidgets(AllWidgets);

	for (UWidget* Widget : AllWidgets)
	{
		if (Widget && Widget->GetName() == WidgetName)
		{
			return Widget;
		}
	}

	return nullptr;
}

// Utility: Convert widget to JSON
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::WidgetToJson(UWidget* Widget)
{
	TSharedPtr<FJsonObject> WidgetJson = MakeShared<FJsonObject>();
	
	if (!Widget)
	{
		return WidgetJson;
	}

	WidgetJson->SetStringField(TEXT("name"), Widget->GetName());
	WidgetJson->SetStringField(TEXT("class"), Widget->GetClass()->GetName());
	WidgetJson->SetBoolField(TEXT("is_visible"), Widget->GetVisibility() == ESlateVisibility::Visible);
	WidgetJson->SetBoolField(TEXT("is_enabled"), Widget->GetIsEnabled());

	// Use reflection to get all properties
	TSharedPtr<FJsonObject> PropertiesObj = MakeShared<FJsonObject>();
	for (TFieldIterator<FProperty> PropIt(Widget->GetClass()); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		if (!Property)
		{
			continue;
		}

		FString PropertyName = Property->GetName();
		void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Widget);

		// Handle different property types
		if (FNumericProperty* NumericProp = CastField<FNumericProperty>(Property))
		{
			if (NumericProp->IsFloatingPoint())
			{
				double Value = NumericProp->GetFloatingPointPropertyValue(ValuePtr);
				PropertiesObj->SetNumberField(PropertyName, Value);
			}
			else if (NumericProp->IsInteger())
			{
				int64 Value = NumericProp->GetSignedIntPropertyValue(ValuePtr);
				PropertiesObj->SetNumberField(PropertyName, static_cast<double>(Value));
			}
		}
		else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
		{
			bool Value = BoolProp->GetPropertyValue(ValuePtr);
			PropertiesObj->SetBoolField(PropertyName, Value);
		}
		else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
		{
			FString Value = StrProp->GetPropertyValue(ValuePtr);
			PropertiesObj->SetStringField(PropertyName, Value);
		}
		else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
		{
			FName Value = NameProp->GetPropertyValue(ValuePtr);
			PropertiesObj->SetStringField(PropertyName, Value.ToString());
		}
		else if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
		{
			FText Value = TextProp->GetPropertyValue(ValuePtr);
			PropertiesObj->SetStringField(PropertyName, Value.ToString());
		}
		else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
		{
			FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
			int64 Value = UnderlyingProp->GetSignedIntPropertyValue(ValuePtr);
			UEnum* EnumType = EnumProp->GetEnum();
			FString EnumValueName = EnumType->GetNameStringByValue(Value);
			PropertiesObj->SetStringField(PropertyName, EnumValueName);
		}
		else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
		{
			UScriptStruct* Struct = StructProp->Struct;
			if (Struct->GetName() == TEXT("Vector2D"))
			{
				FVector2D* Vec = static_cast<FVector2D*>(ValuePtr);
				TSharedPtr<FJsonObject> VecObj = MakeShared<FJsonObject>();
				VecObj->SetNumberField(TEXT("x"), Vec->X);
				VecObj->SetNumberField(TEXT("y"), Vec->Y);
				PropertiesObj->SetObjectField(PropertyName, VecObj);
			}
			else if (Struct->GetName() == TEXT("Vector"))
			{
				FVector* Vec = static_cast<FVector*>(ValuePtr);
				TSharedPtr<FJsonObject> VecObj = MakeShared<FJsonObject>();
				VecObj->SetNumberField(TEXT("x"), Vec->X);
				VecObj->SetNumberField(TEXT("y"), Vec->Y);
				VecObj->SetNumberField(TEXT("z"), Vec->Z);
				PropertiesObj->SetObjectField(PropertyName, VecObj);
			}
			else if (Struct->GetName() == TEXT("LinearColor"))
			{
				FLinearColor* Color = static_cast<FLinearColor*>(ValuePtr);
				TSharedPtr<FJsonObject> ColorObj = MakeShared<FJsonObject>();
				ColorObj->SetNumberField(TEXT("r"), Color->R);
				ColorObj->SetNumberField(TEXT("g"), Color->G);
				ColorObj->SetNumberField(TEXT("b"), Color->B);
				ColorObj->SetNumberField(TEXT("a"), Color->A);
				PropertiesObj->SetObjectField(PropertyName, ColorObj);
			}
			else if (Struct->GetName() == TEXT("SlateColor"))
			{
				FSlateColor* Color = static_cast<FSlateColor*>(ValuePtr);
				FLinearColor SpecifiedColor = Color->GetSpecifiedColor();
				TSharedPtr<FJsonObject> ColorObj = MakeShared<FJsonObject>();
				ColorObj->SetNumberField(TEXT("r"), SpecifiedColor.R);
				ColorObj->SetNumberField(TEXT("g"), SpecifiedColor.G);
				ColorObj->SetNumberField(TEXT("b"), SpecifiedColor.B);
				ColorObj->SetNumberField(TEXT("a"), SpecifiedColor.A);
				PropertiesObj->SetObjectField(PropertyName, ColorObj);
			}
			else if (Struct->GetName() == TEXT("Margin"))
			{
				FMargin* Margin = static_cast<FMargin*>(ValuePtr);
				TSharedPtr<FJsonObject> MarginObj = MakeShared<FJsonObject>();
				MarginObj->SetNumberField(TEXT("left"), Margin->Left);
				MarginObj->SetNumberField(TEXT("top"), Margin->Top);
				MarginObj->SetNumberField(TEXT("right"), Margin->Right);
				MarginObj->SetNumberField(TEXT("bottom"), Margin->Bottom);
				PropertiesObj->SetObjectField(PropertyName, MarginObj);
			}
		}
		else if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
		{
			UObject* ObjValue = ObjProp->GetObjectPropertyValue(ValuePtr);
			if (ObjValue)
			{
				PropertiesObj->SetStringField(PropertyName, ObjValue->GetPathName());
			}
			else
			{
				PropertiesObj->SetStringField(PropertyName, TEXT("None"));
			}
		}
	}

	WidgetJson->SetObjectField(TEXT("properties"), PropertiesObj);

	// Add slot information if widget has a slot
	if (Widget->Slot)
	{
		TSharedPtr<FJsonObject> SlotObj = MakeShared<FJsonObject>();
		SlotObj->SetStringField(TEXT("slot_class"), Widget->Slot->GetClass()->GetName());
		
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
		{
			FVector2D Position = CanvasSlot->GetPosition();
			TSharedPtr<FJsonObject> PosObj = MakeShared<FJsonObject>();
			PosObj->SetNumberField(TEXT("x"), Position.X);
			PosObj->SetNumberField(TEXT("y"), Position.Y);
			SlotObj->SetObjectField(TEXT("position"), PosObj);
			
			FVector2D Size = CanvasSlot->GetSize();
			TSharedPtr<FJsonObject> SizeObj = MakeShared<FJsonObject>();
			SizeObj->SetNumberField(TEXT("x"), Size.X);
			SizeObj->SetNumberField(TEXT("y"), Size.Y);
			SlotObj->SetObjectField(TEXT("size"), SizeObj);
			
			FAnchors Anchors = CanvasSlot->GetAnchors();
			TSharedPtr<FJsonObject> AnchorsObj = MakeShared<FJsonObject>();
			AnchorsObj->SetNumberField(TEXT("min_x"), Anchors.Minimum.X);
			AnchorsObj->SetNumberField(TEXT("min_y"), Anchors.Minimum.Y);
			AnchorsObj->SetNumberField(TEXT("max_x"), Anchors.Maximum.X);
			AnchorsObj->SetNumberField(TEXT("max_y"), Anchors.Maximum.Y);
			SlotObj->SetObjectField(TEXT("anchors"), AnchorsObj);
			
			FVector2D Alignment = CanvasSlot->GetAlignment();
			TSharedPtr<FJsonObject> AlignObj = MakeShared<FJsonObject>();
			AlignObj->SetNumberField(TEXT("x"), Alignment.X);
			AlignObj->SetNumberField(TEXT("y"), Alignment.Y);
			SlotObj->SetObjectField(TEXT("alignment"), AlignObj);
			
			SlotObj->SetNumberField(TEXT("z_order"), CanvasSlot->GetZOrder());
		}
		
		WidgetJson->SetObjectField(TEXT("slot"), SlotObj);
	}

	return WidgetJson;
}

// Utility: Create error response
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::CreateErrorResponse(const FString& ErrorMessage)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("status"), TEXT("error"));
	Response->SetStringField(TEXT("error"), ErrorMessage);
	return Response;
}

// Utility: Create success response
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::CreateSuccessResponse(const FString& Message)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("status"), TEXT("success"));
	Response->SetStringField(TEXT("message"), Message);
	return Response;
}
