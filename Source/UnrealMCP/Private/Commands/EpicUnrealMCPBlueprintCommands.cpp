#include "Commands/EpicUnrealMCPBlueprintCommands.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Factories/BlueprintFactory.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/LightComponent.h"
#include "Components/LightComponentBase.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Engine/Engine.h"
#include "Engine/Light.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Engine/DirectionalLight.h"
#include "Engine/RectLight.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Camera/CameraActor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "UObject/Field.h"
#include "UObject/FieldPath.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "FileHelpers.h"
#include "Exporters/Exporter.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/Skeleton.h"
#include "AnimGraphNode_Slot.h"
#include "Engine/DataTable.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "JsonObjectConverter.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Editor.h"

namespace
{
TArray<TSharedPtr<FJsonValue>> BuildBoneArray(const FReferenceSkeleton& ReferenceSkeleton)
{
    TArray<FString> BoneNames;
    BoneNames.Reserve(ReferenceSkeleton.GetNum());

    for (int32 BoneIndex = 0; BoneIndex < ReferenceSkeleton.GetNum(); ++BoneIndex)
    {
        BoneNames.Add(ReferenceSkeleton.GetBoneName(BoneIndex).ToString());
    }

    BoneNames.Sort();

    TArray<TSharedPtr<FJsonValue>> BoneArray;
    BoneArray.Reserve(BoneNames.Num());
    for (const FString& BoneName : BoneNames)
    {
        BoneArray.Add(MakeShared<FJsonValueString>(BoneName));
    }

    return BoneArray;
}

TSet<FName> BuildBoneSet(const FReferenceSkeleton& ReferenceSkeleton)
{
    TSet<FName> BoneSet;
    for (int32 BoneIndex = 0; BoneIndex < ReferenceSkeleton.GetNum(); ++BoneIndex)
    {
        BoneSet.Add(ReferenceSkeleton.GetBoneName(BoneIndex));
    }
    return BoneSet;
}

TArray<TSharedPtr<FJsonValue>> BuildMissingBoneArray(const FReferenceSkeleton& SourceSkeleton, const TSet<FName>& TargetBones)
{
    TArray<FString> MissingBoneNames;
    for (int32 BoneIndex = 0; BoneIndex < SourceSkeleton.GetNum(); ++BoneIndex)
    {
        const FName BoneName = SourceSkeleton.GetBoneName(BoneIndex);
        if (!TargetBones.Contains(BoneName))
        {
            MissingBoneNames.Add(BoneName.ToString());
        }
    }

    MissingBoneNames.Sort();

    TArray<TSharedPtr<FJsonValue>> MissingBones;
    MissingBones.Reserve(MissingBoneNames.Num());
    for (const FString& BoneName : MissingBoneNames)
    {
        MissingBones.Add(MakeShared<FJsonValueString>(BoneName));
    }

    return MissingBones;
}
}

FEpicUnrealMCPBlueprintCommands::FEpicUnrealMCPBlueprintCommands()
{
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("create_blueprint"))
    {
        return HandleCreateBlueprint(Params);
    }
    else if (CommandType == TEXT("add_component_to_blueprint"))
    {
        return HandleAddComponentToBlueprint(Params);
    }
    else if (CommandType == TEXT("set_physics_properties"))
    {
        return HandleSetPhysicsProperties(Params);
    }
    else if (CommandType == TEXT("compile_blueprint"))
    {
        return HandleCompileBlueprint(Params);
    }
    else if (CommandType == TEXT("set_static_mesh_properties"))
    {
        return HandleSetStaticMeshProperties(Params);
    }
    else if (CommandType == TEXT("spawn_blueprint_actor"))
    {
        return HandleSpawnBlueprintActor(Params);
    }
    else if (CommandType == TEXT("set_mesh_material_color"))
    {
        return HandleSetMeshMaterialColor(Params);
    }
    // Material management commands
    else if (CommandType == TEXT("get_available_materials"))
    {
        return HandleGetAvailableMaterials(Params);
    }
    else if (CommandType == TEXT("apply_material_to_actor"))
    {
        return HandleApplyMaterialToActor(Params);
    }
    else if (CommandType == TEXT("apply_material_to_blueprint"))
    {
        return HandleApplyMaterialToBlueprint(Params);
    }
    else if (CommandType == TEXT("get_actor_material_info"))
    {
        return HandleGetActorMaterialInfo(Params);
    }
    else if (CommandType == TEXT("get_blueprint_material_info"))
    {
        return HandleGetBlueprintMaterialInfo(Params);
    }
    // Blueprint analysis commands
    else if (CommandType == TEXT("read_blueprint_content"))
    {
        return HandleReadBlueprintContent(Params);
    }
    else if (CommandType == TEXT("analyze_blueprint_graph"))
    {
        return HandleAnalyzeBlueprintGraph(Params);
    }
    else if (CommandType == TEXT("get_blueprint_variable_details"))
    {
        return HandleGetBlueprintVariableDetails(Params);
    }
    else if (CommandType == TEXT("get_blueprint_function_details"))
    {
        return HandleGetBlueprintFunctionDetails(Params);
    }
    else if (CommandType == TEXT("fix_duplicate_anim_slots"))
    {
        return HandleFixDuplicateAnimSlots(Params);
    }
    else if (CommandType == TEXT("list_blueprint_graphs"))
    {
        return HandleListBlueprintGraphs(Params);
    }
    else if (CommandType == TEXT("set_animation_blueprint_skeleton"))
    {
        return HandleSetAnimationBlueprintSkeleton(Params);
    }
    else if (CommandType == TEXT("retarget_animation_blueprint"))
    {
        return HandleRetargetAnimationBlueprint(Params);
    }
    else if (CommandType == TEXT("get_skeleton_bones"))
    {
        return HandleGetSkeletonBones(Params);
    }
    else if (CommandType == TEXT("compare_skeleton_bones"))
    {
        return HandleCompareSkeletonBones(Params);
    }
    else if (CommandType == TEXT("get_datatable"))
    {
        return HandleGetDataTable(Params);
    }
    else if (CommandType == TEXT("update_datatable_row"))
    {
        return HandleUpdateDataTableRow(Params);
    }
    else if (CommandType == TEXT("create_datatable"))
    {
        return HandleCreateDataTable(Params);
    }
    else if (CommandType == TEXT("delete_datatable_row"))
    {
        return HandleDeleteDataTableRow(Params);
    }
    else if (CommandType == TEXT("delete_asset"))
    {
        return HandleDeleteAsset(Params);
    }
    // DataTable operations (Extended)
    else if (CommandType == TEXT("add_datatable_row"))
    {
        return HandleAddDataTableRow(Params);
    }
    else if (CommandType == TEXT("rename_datatable_row"))
    {
        return HandleRenameDataTableRow(Params);
    }
    else if (CommandType == TEXT("get_datatable_row_names"))
    {
        return HandleGetDataTableRowNames(Params);
    }
    // Asset operations (Extended)
    else if (CommandType == TEXT("rename_asset"))
    {
        return HandleRenameAsset(Params);
    }
    else if (CommandType == TEXT("duplicate_asset"))
    {
        return HandleDuplicateAsset(Params);
    }
    else if (CommandType == TEXT("move_asset"))
    {
        return HandleMoveAsset(Params);
    }
    else if (CommandType == TEXT("get_asset_metadata"))
    {
        return HandleGetAssetMetadata(Params);
    }
    else if (CommandType == TEXT("list_assets_in_folder"))
    {
        return HandleListAssetsInFolder(Params);
    }
    else if (CommandType == TEXT("save_asset"))
    {
        return HandleSaveAsset(Params);
    }
    // Generic Property Access
    else if (CommandType == TEXT("get_object_property"))
    {
        return HandleGetObjectProperty(Params);
    }
    else if (CommandType == TEXT("set_object_property"))
    {
        return HandleSetObjectProperty(Params);
    }
    else if (CommandType == TEXT("get_object_properties"))
    {
        return HandleGetObjectProperties(Params);
    }
    // Blueprint Component Management
    else if (CommandType == TEXT("get_blueprint_components"))
    {
        return HandleGetBlueprintComponents(Params);
    }
    else if (CommandType == TEXT("remove_component_from_blueprint"))
    {
        return HandleRemoveComponentFromBlueprint(Params);
    }
    else if (CommandType == TEXT("set_component_properties"))
    {
        return HandleSetComponentProperties(Params);
    }
    else if (CommandType == TEXT("get_blueprint_parent_class"))
    {
        return HandleGetBlueprintParentClass(Params);
    }
    // Actor Component Management
    else if (CommandType == TEXT("get_actor_components"))
    {
        return HandleGetActorComponents(Params);
    }
    else if (CommandType == TEXT("add_component_to_actor"))
    {
        return HandleAddComponentToActor(Params);
    }
    else if (CommandType == TEXT("remove_component_from_actor"))
    {
        return HandleRemoveComponentFromActor(Params);
    }
    else if (CommandType == TEXT("get_component_property"))
    {
        return HandleGetComponentProperty(Params);
    }
    else if (CommandType == TEXT("set_component_property"))
    {
        return HandleSetComponentProperty(Params);
    }
    // Material Instance & Parameters
    else if (CommandType == TEXT("create_material_instance"))
    {
        return HandleCreateMaterialInstance(Params);
    }
    else if (CommandType == TEXT("set_material_scalar_parameter"))
    {
        return HandleSetMaterialScalarParameter(Params);
    }
    else if (CommandType == TEXT("set_material_vector_parameter"))
    {
        return HandleSetMaterialVectorParameter(Params);
    }
    else if (CommandType == TEXT("set_material_texture_parameter"))
    {
        return HandleSetMaterialTextureParameter(Params);
    }
    else if (CommandType == TEXT("get_material_parameters"))
    {
        return HandleGetMaterialParameters(Params);
    }
    // Level Management
    else if (CommandType == TEXT("save_current_level"))
    {
        return HandleSaveCurrentLevel(Params);
    }
    else if (CommandType == TEXT("load_level"))
    {
        return HandleLoadLevel(Params);
    }
    else if (CommandType == TEXT("get_current_level_name"))
    {
        return HandleGetCurrentLevelName(Params);
    }
    else if (CommandType == TEXT("get_all_levels"))
    {
        return HandleGetAllLevels(Params);
    }
    // Lighting & Cameras
    else if (CommandType == TEXT("spawn_light"))
    {
        return HandleSpawnLight(Params);
    }
    else if (CommandType == TEXT("set_light_properties"))
    {
        return HandleSetLightProperties(Params);
    }
    else if (CommandType == TEXT("spawn_camera"))
    {
        return HandleSpawnCamera(Params);
    }
    else if (CommandType == TEXT("set_camera_properties"))
    {
        return HandleSetCameraProperties(Params);
    }
    // Collision & Physics
    else if (CommandType == TEXT("set_actor_collision_preset"))
    {
        return HandleSetActorCollisionPreset(Params);
    }
    else if (CommandType == TEXT("set_collision_enabled"))
    {
        return HandleSetCollisionEnabled(Params);
    }
    // Import/Export
    else if (CommandType == TEXT("import_asset"))
    {
        return HandleImportAsset(Params);
    }
    else if (CommandType == TEXT("export_asset"))
    {
        return HandleExportAsset(Params);
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown blueprint command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleCreateBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Check if blueprint already exists
    FString PackagePath = TEXT("/Game/Blueprints/");
    FString AssetName = BlueprintName;
    if (UEditorAssetLibrary::DoesAssetExist(PackagePath + AssetName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint already exists: %s"), *BlueprintName));
    }

    // Create the blueprint factory
    UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
    
    // Handle parent class
    FString ParentClass;
    Params->TryGetStringField(TEXT("parent_class"), ParentClass);
    
    // Default to Actor if no parent class specified
    UClass* SelectedParentClass = AActor::StaticClass();
    
    // Try to find the specified parent class
    if (!ParentClass.IsEmpty())
    {
        FString ClassName = ParentClass;
        if (!ClassName.StartsWith(TEXT("A")))
        {
            ClassName = TEXT("A") + ClassName;
        }
        
        // First try direct StaticClass lookup for common classes
        UClass* FoundClass = nullptr;
        if (ClassName == TEXT("APawn"))
        {
            FoundClass = APawn::StaticClass();
        }
        else if (ClassName == TEXT("AActor"))
        {
            FoundClass = AActor::StaticClass();
        }
        else
        {
            // Try loading the class using LoadClass which is more reliable than FindObject
            const FString ClassPath = FString::Printf(TEXT("/Script/Engine.%s"), *ClassName);
            FoundClass = LoadClass<AActor>(nullptr, *ClassPath);
            
            if (!FoundClass)
            {
                // Try alternate paths if not found
                const FString GameClassPath = FString::Printf(TEXT("/Script/Game.%s"), *ClassName);
                FoundClass = LoadClass<AActor>(nullptr, *GameClassPath);
            }
        }

        if (FoundClass)
        {
            SelectedParentClass = FoundClass;
            UE_LOG(LogTemp, Log, TEXT("Successfully set parent class to '%s'"), *ClassName);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Could not find specified parent class '%s' at paths: /Script/Engine.%s or /Script/Game.%s, defaulting to AActor"), 
                *ClassName, *ClassName, *ClassName);
        }
    }
    
    Factory->ParentClass = SelectedParentClass;

    // Create the blueprint
    UPackage* Package = CreatePackage(*(PackagePath + AssetName));
    UBlueprint* NewBlueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(UBlueprint::StaticClass(), Package, *AssetName, RF_Standalone | RF_Public, nullptr, GWarn));

    if (NewBlueprint)
    {
        // Notify the asset registry
        FAssetRegistryModule::AssetCreated(NewBlueprint);

        // Mark the package dirty
        Package->MarkPackageDirty();

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("name"), AssetName);
        ResultObj->SetStringField(TEXT("path"), PackagePath + AssetName);
        return ResultObj;
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create blueprint"));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleAddComponentToBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentType;
    if (!Params->TryGetStringField(TEXT("component_type"), ComponentType))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'type' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Create the component - dynamically find the component class by name
    UClass* ComponentClass = nullptr;

    // Try to find the class with exact name first
    ComponentClass = FindObject<UClass>(nullptr, *ComponentType);
    
    // If not found, try with "Component" suffix
    if (!ComponentClass && !ComponentType.EndsWith(TEXT("Component")))
    {
        FString ComponentTypeWithSuffix = ComponentType + TEXT("Component");
        ComponentClass = FindObject<UClass>(nullptr, *ComponentTypeWithSuffix);
    }
    
    // If still not found, try with "U" prefix
    if (!ComponentClass && !ComponentType.StartsWith(TEXT("U")))
    {
        FString ComponentTypeWithPrefix = TEXT("U") + ComponentType;
        ComponentClass = FindObject<UClass>(nullptr, *ComponentTypeWithPrefix);
        
        // Try with both prefix and suffix
        if (!ComponentClass && !ComponentType.EndsWith(TEXT("Component")))
        {
            FString ComponentTypeWithBoth = TEXT("U") + ComponentType + TEXT("Component");
            ComponentClass = FindObject<UClass>(nullptr, *ComponentTypeWithBoth);
        }
    }
    
    // Verify that the class is a valid component type
    if (!ComponentClass || !ComponentClass->IsChildOf(UActorComponent::StaticClass()))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown component type: %s"), *ComponentType));
    }

    // Add the component to the blueprint
    USCS_Node* NewNode = Blueprint->SimpleConstructionScript->CreateNode(ComponentClass, *ComponentName);
    if (NewNode)
    {
        // Set transform if provided
        USceneComponent* SceneComponent = Cast<USceneComponent>(NewNode->ComponentTemplate);
        if (SceneComponent)
        {
            if (Params->HasField(TEXT("location")))
            {
                SceneComponent->SetRelativeLocation(FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location")));
            }
            if (Params->HasField(TEXT("rotation")))
            {
                SceneComponent->SetRelativeRotation(FEpicUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation")));
            }
            if (Params->HasField(TEXT("scale")))
            {
                SceneComponent->SetRelativeScale3D(FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale")));
            }
        }

        // Add to root if no parent specified
        Blueprint->SimpleConstructionScript->AddNode(NewNode);

        // Compile the blueprint
        FKismetEditorUtilities::CompileBlueprint(Blueprint);

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("component_name"), ComponentName);
        ResultObj->SetStringField(TEXT("component_type"), ComponentType);
        return ResultObj;
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to add component to blueprint"));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetPhysicsProperties(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Find the component
    USCS_Node* ComponentNode = nullptr;
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (Node && Node->GetVariableName().ToString() == ComponentName)
        {
            ComponentNode = Node;
            break;
        }
    }

    if (!ComponentNode)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
    }

    UPrimitiveComponent* PrimComponent = Cast<UPrimitiveComponent>(ComponentNode->ComponentTemplate);
    if (!PrimComponent)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Component is not a primitive component"));
    }

    // Set physics properties
    if (Params->HasField(TEXT("simulate_physics")))
    {
        PrimComponent->SetSimulatePhysics(Params->GetBoolField(TEXT("simulate_physics")));
    }

    if (Params->HasField(TEXT("mass")))
    {
        float Mass = Params->GetNumberField(TEXT("mass"));
        // In UE5.5, use proper overrideMass instead of just scaling
        PrimComponent->SetMassOverrideInKg(NAME_None, Mass);
        UE_LOG(LogTemp, Display, TEXT("Set mass for component %s to %f kg"), *ComponentName, Mass);
    }

    if (Params->HasField(TEXT("linear_damping")))
    {
        PrimComponent->SetLinearDamping(Params->GetNumberField(TEXT("linear_damping")));
    }

    if (Params->HasField(TEXT("angular_damping")))
    {
        PrimComponent->SetAngularDamping(Params->GetNumberField(TEXT("angular_damping")));
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("component"), ComponentName);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleCompileBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Compile the blueprint
    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("name"), BlueprintName);
    ResultObj->SetBoolField(TEXT("compiled"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params)
{
    UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Starting blueprint actor spawn"));
    
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        UE_LOG(LogTemp, Error, TEXT("HandleSpawnBlueprintActor: Missing blueprint_name parameter"));
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        UE_LOG(LogTemp, Error, TEXT("HandleSpawnBlueprintActor: Missing actor_name parameter"));
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Looking for blueprint '%s'"), *BlueprintName);

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("HandleSpawnBlueprintActor: Blueprint not found: %s"), *BlueprintName);
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Blueprint found, getting transform parameters"));

    // Get transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
        UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Location set to (%f, %f, %f)"), Location.X, Location.Y, Location.Z);
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FEpicUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
        UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Rotation set to (%f, %f, %f)"), Rotation.Pitch, Rotation.Yaw, Rotation.Roll);
    }

    UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Getting editor world"));

    // Spawn the actor
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("HandleSpawnBlueprintActor: Failed to get editor world"));
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Creating spawn transform"));

    FTransform SpawnTransform;
    SpawnTransform.SetLocation(Location);
    SpawnTransform.SetRotation(FQuat(Rotation));

    // Add a small delay to allow the engine to process the newly compiled class
    FPlatformProcess::Sleep(0.2f);

    UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: About to spawn actor from blueprint '%s' with GeneratedClass: %s"), 
           *BlueprintName, Blueprint->GeneratedClass ? *Blueprint->GeneratedClass->GetName() : TEXT("NULL"));

    AActor* NewActor = World->SpawnActor<AActor>(Blueprint->GeneratedClass, SpawnTransform);
    
    UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: SpawnActor completed, NewActor: %s"), 
           NewActor ? *NewActor->GetName() : TEXT("NULL"));
    
    if (NewActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Setting actor label to '%s'"), *ActorName);
        NewActor->SetActorLabel(*ActorName);
        
        UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: About to convert actor to JSON"));
        TSharedPtr<FJsonObject> Result = FEpicUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
        
        UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: JSON conversion completed, returning result"));
        return Result;
    }

    UE_LOG(LogTemp, Error, TEXT("HandleSpawnBlueprintActor: Failed to spawn blueprint actor"));
    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to spawn blueprint actor"));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetStaticMeshProperties(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Find the component
    USCS_Node* ComponentNode = nullptr;
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (Node && Node->GetVariableName().ToString() == ComponentName)
        {
            ComponentNode = Node;
            break;
        }
    }

    if (!ComponentNode)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
    }

    UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(ComponentNode->ComponentTemplate);
    if (!MeshComponent)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Component is not a static mesh component"));
    }

    // Set static mesh properties
    if (Params->HasField(TEXT("static_mesh")))
    {
        FString MeshPath = Params->GetStringField(TEXT("static_mesh"));
        UStaticMesh* Mesh = Cast<UStaticMesh>(UEditorAssetLibrary::LoadAsset(MeshPath));
        if (Mesh)
        {
            MeshComponent->SetStaticMesh(Mesh);
        }
    }

    if (Params->HasField(TEXT("material")))
    {
        FString MaterialPath = Params->GetStringField(TEXT("material"));
        UMaterialInterface* Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
        if (Material)
        {
            MeshComponent->SetMaterial(0, Material);
        }
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("component"), ComponentName);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetMeshMaterialColor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Find the component
    USCS_Node* ComponentNode = nullptr;
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (Node && Node->GetVariableName().ToString() == ComponentName)
        {
            ComponentNode = Node;
            break;
        }
    }

    if (!ComponentNode)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
    }

    // Try to cast to StaticMeshComponent or PrimitiveComponent
    UPrimitiveComponent* PrimComponent = Cast<UPrimitiveComponent>(ComponentNode->ComponentTemplate);
    if (!PrimComponent)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Component is not a primitive component"));
    }

    // Get color parameter
    TArray<float> ColorArray;
    const TArray<TSharedPtr<FJsonValue>>* ColorJsonArray;
    if (!Params->TryGetArrayField(TEXT("color"), ColorJsonArray) || ColorJsonArray->Num() != 4)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("'color' must be an array of 4 float values [R, G, B, A]"));
    }

    for (const TSharedPtr<FJsonValue>& Value : *ColorJsonArray)
    {
        ColorArray.Add(FMath::Clamp(Value->AsNumber(), 0.0f, 1.0f));
    }

    FLinearColor Color(ColorArray[0], ColorArray[1], ColorArray[2], ColorArray[3]);

    // Get material slot index
    int32 MaterialSlot = 0;
    if (Params->HasField(TEXT("material_slot")))
    {
        MaterialSlot = Params->GetIntegerField(TEXT("material_slot"));
    }

    // Get parameter name
    FString ParameterName = TEXT("BaseColor");
    Params->TryGetStringField(TEXT("parameter_name"), ParameterName);

    // Get or create material
    UMaterialInterface* Material = nullptr;
    
    // Check if a specific material path was provided
    FString MaterialPath;
    if (Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
        if (!Material)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
        }
    }
    else
    {
        // Use existing material on the component
        Material = PrimComponent->GetMaterial(MaterialSlot);
        if (!Material)
        {
            // Try to use a default material
            Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial")));
            if (!Material)
            {
                return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No material found on component and failed to load default material"));
            }
        }
    }

    // Create a dynamic material instance
    UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(Material, PrimComponent);
    if (!DynMaterial)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create dynamic material instance"));
    }

    // Set the color parameter
    DynMaterial->SetVectorParameterValue(*ParameterName, Color);

    // Apply the material to the component
    PrimComponent->SetMaterial(MaterialSlot, DynMaterial);

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    // Log success
    UE_LOG(LogTemp, Log, TEXT("Successfully set material color on component %s: R=%f, G=%f, B=%f, A=%f"), 
        *ComponentName, Color.R, Color.G, Color.B, Color.A);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("component"), ComponentName);
    ResultObj->SetNumberField(TEXT("material_slot"), MaterialSlot);
    ResultObj->SetStringField(TEXT("parameter_name"), ParameterName);
    
    TArray<TSharedPtr<FJsonValue>> ColorResultArray;
    ColorResultArray.Add(MakeShared<FJsonValueNumber>(Color.R));
    ColorResultArray.Add(MakeShared<FJsonValueNumber>(Color.G));
    ColorResultArray.Add(MakeShared<FJsonValueNumber>(Color.B));
    ColorResultArray.Add(MakeShared<FJsonValueNumber>(Color.A));
    ResultObj->SetArrayField(TEXT("color"), ColorResultArray);
    
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetAvailableMaterials(const TSharedPtr<FJsonObject>& Params)
{
    // Get parameters - make search path completely dynamic
    FString SearchPath;
    if (!Params->TryGetStringField(TEXT("search_path"), SearchPath))
    {
        // Default to empty string to search everywhere
        SearchPath = TEXT("");
    }
    
    bool bIncludeEngineMaterials = true;
    if (Params->HasField(TEXT("include_engine_materials")))
    {
        bIncludeEngineMaterials = Params->GetBoolField(TEXT("include_engine_materials"));
    }

    // Get asset registry module
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    // Create filter for materials
    FARFilter Filter;
    Filter.ClassPaths.Add(UMaterialInterface::StaticClass()->GetClassPathName());
    Filter.ClassPaths.Add(UMaterial::StaticClass()->GetClassPathName());
    Filter.ClassPaths.Add(UMaterialInstanceConstant::StaticClass()->GetClassPathName());
    Filter.ClassPaths.Add(UMaterialInstanceDynamic::StaticClass()->GetClassPathName());
    
    // Add search paths dynamically
    if (!SearchPath.IsEmpty())
    {
        // Ensure the path starts with /
        if (!SearchPath.StartsWith(TEXT("/")))
        {
            SearchPath = TEXT("/") + SearchPath;
        }
        // Ensure the path ends with / for proper directory search
        if (!SearchPath.EndsWith(TEXT("/")))
        {
            SearchPath += TEXT("/");
        }
        Filter.PackagePaths.Add(*SearchPath);
        UE_LOG(LogTemp, Log, TEXT("Searching for materials in: %s"), *SearchPath);
    }
    else
    {
        // Search in common game content locations
        Filter.PackagePaths.Add(TEXT("/Game/"));
        UE_LOG(LogTemp, Log, TEXT("Searching for materials in all game content"));
    }
    
    if (bIncludeEngineMaterials)
    {
        Filter.PackagePaths.Add(TEXT("/Engine/"));
        UE_LOG(LogTemp, Log, TEXT("Including Engine materials in search"));
    }
    
    Filter.bRecursivePaths = true;

    // Get assets from registry
    TArray<FAssetData> AssetDataArray;
    AssetRegistry.GetAssets(Filter, AssetDataArray);
    
    UE_LOG(LogTemp, Log, TEXT("Asset registry found %d materials"), AssetDataArray.Num());

    // Also try manual search using EditorAssetLibrary for more comprehensive results
    TArray<FString> AllAssetPaths;
    if (!SearchPath.IsEmpty())
    {
        AllAssetPaths = UEditorAssetLibrary::ListAssets(SearchPath, true, false);
    }
    else
    {
        AllAssetPaths = UEditorAssetLibrary::ListAssets(TEXT("/Game/"), true, false);
    }
    
    // Filter for materials from the manual search
    for (const FString& AssetPath : AllAssetPaths)
    {
        if (AssetPath.Contains(TEXT("Material")) && !AssetPath.Contains(TEXT(".uasset")))
        {
            UObject* Asset = UEditorAssetLibrary::LoadAsset(AssetPath);
            if (Asset && Asset->IsA<UMaterialInterface>())
            {
                // Check if we already have this asset from registry search
                bool bAlreadyFound = false;
                for (const FAssetData& ExistingData : AssetDataArray)
                {
                    if (ExistingData.GetObjectPathString() == AssetPath)
                    {
                        bAlreadyFound = true;
                        break;
                    }
                }
                
                if (!bAlreadyFound)
                {
                    // Create FAssetData manually for this asset
                    FAssetData ManualAssetData(Asset);
                    AssetDataArray.Add(ManualAssetData);
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Total materials found after manual search: %d"), AssetDataArray.Num());

    // Convert to JSON
    TArray<TSharedPtr<FJsonValue>> MaterialArray;
    for (const FAssetData& AssetData : AssetDataArray)
    {
        TSharedPtr<FJsonObject> MaterialObj = MakeShared<FJsonObject>();
        MaterialObj->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
        MaterialObj->SetStringField(TEXT("path"), AssetData.GetObjectPathString());
        MaterialObj->SetStringField(TEXT("package"), AssetData.PackageName.ToString());
        MaterialObj->SetStringField(TEXT("class"), AssetData.AssetClassPath.ToString());
        
        MaterialArray.Add(MakeShared<FJsonValueObject>(MaterialObj));
        
        UE_LOG(LogTemp, Verbose, TEXT("Found material: %s at %s"), *AssetData.AssetName.ToString(), *AssetData.GetObjectPathString());
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("materials"), MaterialArray);
    ResultObj->SetNumberField(TEXT("count"), MaterialArray.Num());
    ResultObj->SetStringField(TEXT("search_path_used"), SearchPath.IsEmpty() ? TEXT("/Game/") : SearchPath);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleApplyMaterialToActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    int32 MaterialSlot = 0;
    if (Params->HasField(TEXT("material_slot")))
    {
        MaterialSlot = Params->GetIntegerField(TEXT("material_slot"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }
    
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Load the material
    UMaterialInterface* Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
    }

    // Find mesh components and apply material
    TArray<UStaticMeshComponent*> MeshComponents;
    TargetActor->GetComponents<UStaticMeshComponent>(MeshComponents);
    
    bool bAppliedToAny = false;
    for (UStaticMeshComponent* MeshComp : MeshComponents)
    {
        if (MeshComp)
        {
            MeshComp->SetMaterial(MaterialSlot, Material);
            bAppliedToAny = true;
        }
    }

    if (!bAppliedToAny)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No mesh components found on actor"));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("actor_name"), ActorName);
    ResultObj->SetStringField(TEXT("material_path"), MaterialPath);
    ResultObj->SetNumberField(TEXT("material_slot"), MaterialSlot);
    ResultObj->SetBoolField(TEXT("success"), true);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleApplyMaterialToBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    int32 MaterialSlot = 0;
    if (Params->HasField(TEXT("material_slot")))
    {
        MaterialSlot = Params->GetIntegerField(TEXT("material_slot"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Find the component
    USCS_Node* ComponentNode = nullptr;
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (Node && Node->GetVariableName().ToString() == ComponentName)
        {
            ComponentNode = Node;
            break;
        }
    }

    if (!ComponentNode)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
    }

    UPrimitiveComponent* PrimComponent = Cast<UPrimitiveComponent>(ComponentNode->ComponentTemplate);
    if (!PrimComponent)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Component is not a primitive component"));
    }

    // Load the material
    UMaterialInterface* Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
    }

    // Apply the material
    PrimComponent->SetMaterial(MaterialSlot, Material);

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResultObj->SetStringField(TEXT("component_name"), ComponentName);
    ResultObj->SetStringField(TEXT("material_path"), MaterialPath);
    ResultObj->SetNumberField(TEXT("material_slot"), MaterialSlot);
    ResultObj->SetBoolField(TEXT("success"), true);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetActorMaterialInfo(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }
    
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get mesh components and their materials
    TArray<UStaticMeshComponent*> MeshComponents;
    TargetActor->GetComponents<UStaticMeshComponent>(MeshComponents);
    
    TArray<TSharedPtr<FJsonValue>> MaterialSlots;
    
    for (UStaticMeshComponent* MeshComp : MeshComponents)
    {
        if (MeshComp)
        {
            for (int32 i = 0; i < MeshComp->GetNumMaterials(); i++)
            {
                TSharedPtr<FJsonObject> SlotInfo = MakeShared<FJsonObject>();
                SlotInfo->SetNumberField(TEXT("slot"), i);
                SlotInfo->SetStringField(TEXT("component"), MeshComp->GetName());
                
                UMaterialInterface* Material = MeshComp->GetMaterial(i);
                if (Material)
                {
                    SlotInfo->SetStringField(TEXT("material_name"), Material->GetName());
                    SlotInfo->SetStringField(TEXT("material_path"), Material->GetPathName());
                    SlotInfo->SetStringField(TEXT("material_class"), Material->GetClass()->GetName());
                }
                else
                {
                    SlotInfo->SetStringField(TEXT("material_name"), TEXT("None"));
                    SlotInfo->SetStringField(TEXT("material_path"), TEXT(""));
                    SlotInfo->SetStringField(TEXT("material_class"), TEXT(""));
                }
                
                MaterialSlots.Add(MakeShared<FJsonValueObject>(SlotInfo));
            }
        }
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("actor_name"), ActorName);
    ResultObj->SetArrayField(TEXT("material_slots"), MaterialSlots);
    ResultObj->SetNumberField(TEXT("total_slots"), MaterialSlots.Num());
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetBlueprintMaterialInfo(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Find the component
    USCS_Node* ComponentNode = nullptr;
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (Node && Node->GetVariableName().ToString() == ComponentName)
        {
            ComponentNode = Node;
            break;
        }
    }

    if (!ComponentNode)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
    }

    UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(ComponentNode->ComponentTemplate);
    if (!MeshComponent)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Component is not a static mesh component"));
    }

    // Get material slot information
    TArray<TSharedPtr<FJsonValue>> MaterialSlots;
    int32 NumMaterials = 0;
    
    // Check if we have a static mesh assigned
    UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh();
    if (StaticMesh)
    {
        NumMaterials = StaticMesh->GetNumSections(0); // Get number of material slots for LOD 0
        
        for (int32 i = 0; i < NumMaterials; i++)
        {
            TSharedPtr<FJsonObject> SlotInfo = MakeShared<FJsonObject>();
            SlotInfo->SetNumberField(TEXT("slot"), i);
            SlotInfo->SetStringField(TEXT("component"), ComponentName);
            
            UMaterialInterface* Material = MeshComponent->GetMaterial(i);
            if (Material)
            {
                SlotInfo->SetStringField(TEXT("material_name"), Material->GetName());
                SlotInfo->SetStringField(TEXT("material_path"), Material->GetPathName());
                SlotInfo->SetStringField(TEXT("material_class"), Material->GetClass()->GetName());
            }
            else
            {
                SlotInfo->SetStringField(TEXT("material_name"), TEXT("None"));
                SlotInfo->SetStringField(TEXT("material_path"), TEXT(""));
                SlotInfo->SetStringField(TEXT("material_class"), TEXT(""));
            }
            
            MaterialSlots.Add(MakeShared<FJsonValueObject>(SlotInfo));
        }
    }
    else
    {
        // If no static mesh is assigned, we can't determine material slots
        UE_LOG(LogTemp, Warning, TEXT("No static mesh assigned to component %s in blueprint %s"), *ComponentName, *BlueprintName);
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResultObj->SetStringField(TEXT("component_name"), ComponentName);
    ResultObj->SetArrayField(TEXT("material_slots"), MaterialSlots);
    ResultObj->SetNumberField(TEXT("total_slots"), MaterialSlots.Num());
    ResultObj->SetBoolField(TEXT("has_static_mesh"), StaticMesh != nullptr);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleReadBlueprintContent(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
    }

    // Get optional parameters
    bool bIncludeEventGraph = true;
    bool bIncludeFunctions = true;
    bool bIncludeVariables = true;
    bool bIncludeComponents = true;
    bool bIncludeInterfaces = true;

    Params->TryGetBoolField(TEXT("include_event_graph"), bIncludeEventGraph);
    Params->TryGetBoolField(TEXT("include_functions"), bIncludeFunctions);
    Params->TryGetBoolField(TEXT("include_variables"), bIncludeVariables);
    Params->TryGetBoolField(TEXT("include_components"), bIncludeComponents);
    Params->TryGetBoolField(TEXT("include_interfaces"), bIncludeInterfaces);

    // Load the blueprint
    UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint_path"), BlueprintPath);
    ResultObj->SetStringField(TEXT("blueprint_name"), Blueprint->GetName());
    ResultObj->SetStringField(TEXT("parent_class"), Blueprint->ParentClass ? Blueprint->ParentClass->GetName() : TEXT("None"));

    // Include variables if requested
    if (bIncludeVariables)
    {
        TArray<TSharedPtr<FJsonValue>> VariableArray;
        for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
        {
            TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();
            VarObj->SetStringField(TEXT("name"), Variable.VarName.ToString());
            VarObj->SetStringField(TEXT("type"), Variable.VarType.PinCategory.ToString());
            VarObj->SetStringField(TEXT("default_value"), Variable.DefaultValue);
            VarObj->SetBoolField(TEXT("is_editable"), (Variable.PropertyFlags & CPF_Edit) != 0);
            VariableArray.Add(MakeShared<FJsonValueObject>(VarObj));
        }
        ResultObj->SetArrayField(TEXT("variables"), VariableArray);
    }

    // Include functions if requested
    if (bIncludeFunctions)
    {
        TArray<TSharedPtr<FJsonValue>> FunctionArray;
        for (UEdGraph* Graph : Blueprint->FunctionGraphs)
        {
            if (Graph)
            {
                TSharedPtr<FJsonObject> FuncObj = MakeShared<FJsonObject>();
                FuncObj->SetStringField(TEXT("name"), Graph->GetName());
                FuncObj->SetStringField(TEXT("graph_type"), TEXT("Function"));
                
                // Count nodes in function
                int32 NodeCount = Graph->Nodes.Num();
                FuncObj->SetNumberField(TEXT("node_count"), NodeCount);
                
                FunctionArray.Add(MakeShared<FJsonValueObject>(FuncObj));
            }
        }
        ResultObj->SetArrayField(TEXT("functions"), FunctionArray);
    }

    // Include event graph if requested
    if (bIncludeEventGraph)
    {
        TSharedPtr<FJsonObject> EventGraphObj = MakeShared<FJsonObject>();
        
        // Find the main event graph
        for (UEdGraph* Graph : Blueprint->UbergraphPages)
        {
            if (Graph && Graph->GetName() == TEXT("EventGraph"))
            {
                EventGraphObj->SetStringField(TEXT("name"), Graph->GetName());
                EventGraphObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
                
                // Get basic node information
                TArray<TSharedPtr<FJsonValue>> NodeArray;
                for (UEdGraphNode* Node : Graph->Nodes)
                {
                    if (Node)
                    {
                        TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
                        NodeObj->SetStringField(TEXT("name"), Node->GetName());
                        NodeObj->SetStringField(TEXT("class"), Node->GetClass()->GetName());
                        NodeObj->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
                        NodeArray.Add(MakeShared<FJsonValueObject>(NodeObj));
                    }
                }
                EventGraphObj->SetArrayField(TEXT("nodes"), NodeArray);
                break;
            }
        }
        
        ResultObj->SetObjectField(TEXT("event_graph"), EventGraphObj);
    }

    // Include components if requested
    if (bIncludeComponents)
    {
        TArray<TSharedPtr<FJsonValue>> ComponentArray;
        if (Blueprint->SimpleConstructionScript)
        {
            for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
            {
                if (Node && Node->ComponentTemplate)
                {
                    TSharedPtr<FJsonObject> CompObj = MakeShared<FJsonObject>();
                    CompObj->SetStringField(TEXT("name"), Node->GetVariableName().ToString());
                    CompObj->SetStringField(TEXT("class"), Node->ComponentTemplate->GetClass()->GetName());
                    CompObj->SetBoolField(TEXT("is_root"), Node == Blueprint->SimpleConstructionScript->GetDefaultSceneRootNode());
                    ComponentArray.Add(MakeShared<FJsonValueObject>(CompObj));
                }
            }
        }
        ResultObj->SetArrayField(TEXT("components"), ComponentArray);
    }

    // Include interfaces if requested
    if (bIncludeInterfaces)
    {
        TArray<TSharedPtr<FJsonValue>> InterfaceArray;
        for (const FBPInterfaceDescription& Interface : Blueprint->ImplementedInterfaces)
        {
            TSharedPtr<FJsonObject> InterfaceObj = MakeShared<FJsonObject>();
            InterfaceObj->SetStringField(TEXT("name"), Interface.Interface ? Interface.Interface->GetName() : TEXT("Unknown"));
            InterfaceArray.Add(MakeShared<FJsonValueObject>(InterfaceObj));
        }
        ResultObj->SetArrayField(TEXT("interfaces"), InterfaceArray);
    }

    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleAnalyzeBlueprintGraph(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
    }

    FString GraphName = TEXT("EventGraph");
    Params->TryGetStringField(TEXT("graph_name"), GraphName);

    // Get optional parameters
    bool bIncludeNodeDetails = true;
    bool bIncludePinConnections = true;
    bool bTraceExecutionFlow = true;

    Params->TryGetBoolField(TEXT("include_node_details"), bIncludeNodeDetails);
    Params->TryGetBoolField(TEXT("include_pin_connections"), bIncludePinConnections);
    Params->TryGetBoolField(TEXT("trace_execution_flow"), bTraceExecutionFlow);

    // Load the blueprint
    UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
    }

    // Find the specified graph
    UEdGraph* TargetGraph = nullptr;
    
    // Check event graphs first
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph && Graph->GetName() == GraphName)
        {
            TargetGraph = Graph;
            break;
        }
    }
    
    // Check function graphs if not found
    if (!TargetGraph)
    {
        for (UEdGraph* Graph : Blueprint->FunctionGraphs)
        {
            if (Graph && Graph->GetName() == GraphName)
            {
                TargetGraph = Graph;
                break;
            }
        }
    }

    // Fall back to GetAllGraphs() to catch nested sub-graphs (state machine states, blend trees, etc.)
    if (!TargetGraph)
    {
        TArray<UEdGraph*> AllGraphs;
        Blueprint->GetAllGraphs(AllGraphs);
        for (UEdGraph* Graph : AllGraphs)
        {
            if (Graph && Graph->GetName() == GraphName)
            {
                TargetGraph = Graph;
                break;
            }
        }
    }

    if (!TargetGraph)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Graph not found: %s"), *GraphName));
    }

    TSharedPtr<FJsonObject> GraphData = MakeShared<FJsonObject>();
    GraphData->SetStringField(TEXT("graph_name"), TargetGraph->GetName());
    GraphData->SetStringField(TEXT("graph_type"), TargetGraph->GetClass()->GetName());

    // Analyze nodes
    TArray<TSharedPtr<FJsonValue>> NodeArray;
    TArray<TSharedPtr<FJsonValue>> ConnectionArray;

    for (UEdGraphNode* Node : TargetGraph->Nodes)
    {
        if (Node)
        {
            TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
            NodeObj->SetStringField(TEXT("name"), Node->GetName());
            NodeObj->SetStringField(TEXT("class"), Node->GetClass()->GetName());
            NodeObj->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());

            if (bIncludeNodeDetails)
            {
                NodeObj->SetNumberField(TEXT("pos_x"), Node->NodePosX);
                NodeObj->SetNumberField(TEXT("pos_y"), Node->NodePosY);
                NodeObj->SetBoolField(TEXT("can_rename"), Node->bCanRenameNode);
            }

            // Include pin information if requested
            if (bIncludePinConnections)
            {
                TArray<TSharedPtr<FJsonValue>> PinArray;
                for (UEdGraphPin* Pin : Node->Pins)
                {
                    if (Pin)
                    {
                        TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
                        PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
                        PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
                        PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("Input") : TEXT("Output"));
                        PinObj->SetNumberField(TEXT("connections"), Pin->LinkedTo.Num());
                        
                        // Record connections for this pin
                        for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                        {
                            if (LinkedPin && LinkedPin->GetOwningNode())
                            {
                                TSharedPtr<FJsonObject> ConnObj = MakeShared<FJsonObject>();
                                ConnObj->SetStringField(TEXT("from_node"), Pin->GetOwningNode()->GetName());
                                ConnObj->SetStringField(TEXT("from_pin"), Pin->PinName.ToString());
                                ConnObj->SetStringField(TEXT("to_node"), LinkedPin->GetOwningNode()->GetName());
                                ConnObj->SetStringField(TEXT("to_pin"), LinkedPin->PinName.ToString());
                                ConnectionArray.Add(MakeShared<FJsonValueObject>(ConnObj));
                            }
                        }
                        
                        PinArray.Add(MakeShared<FJsonValueObject>(PinObj));
                    }
                }
                NodeObj->SetArrayField(TEXT("pins"), PinArray);
            }

            NodeArray.Add(MakeShared<FJsonValueObject>(NodeObj));
        }
    }

    GraphData->SetArrayField(TEXT("nodes"), NodeArray);
    GraphData->SetArrayField(TEXT("connections"), ConnectionArray);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint_path"), BlueprintPath);
    ResultObj->SetObjectField(TEXT("graph_data"), GraphData);
    ResultObj->SetBoolField(TEXT("success"), true);

    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetBlueprintVariableDetails(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
    }

    FString VariableName;
    bool bSpecificVariable = Params->TryGetStringField(TEXT("variable_name"), VariableName);

    // Load the blueprint
    UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
    }

    TArray<TSharedPtr<FJsonValue>> VariableArray;

    for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        // If looking for specific variable, skip others
        if (bSpecificVariable && Variable.VarName.ToString() != VariableName)
        {
            continue;
        }

        TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();
        VarObj->SetStringField(TEXT("name"), Variable.VarName.ToString());
        VarObj->SetStringField(TEXT("type"), Variable.VarType.PinCategory.ToString());
        VarObj->SetStringField(TEXT("sub_category"), Variable.VarType.PinSubCategory.ToString());
        VarObj->SetStringField(TEXT("default_value"), Variable.DefaultValue);
        VarObj->SetStringField(TEXT("friendly_name"), Variable.FriendlyName.IsEmpty() ? Variable.VarName.ToString() : Variable.FriendlyName);
        
        // Get tooltip from metadata (VarTooltip doesn't exist in UE 5.5)
        FString TooltipValue;
        if (Variable.HasMetaData(FBlueprintMetadata::MD_Tooltip))
        {
            TooltipValue = Variable.GetMetaData(FBlueprintMetadata::MD_Tooltip);
        }
        VarObj->SetStringField(TEXT("tooltip"), TooltipValue);
        
        VarObj->SetStringField(TEXT("category"), Variable.Category.ToString());

        // Property flags
        VarObj->SetBoolField(TEXT("is_editable"), (Variable.PropertyFlags & CPF_Edit) != 0);
        VarObj->SetBoolField(TEXT("is_blueprint_visible"), (Variable.PropertyFlags & CPF_BlueprintVisible) != 0);
        VarObj->SetBoolField(TEXT("is_editable_in_instance"), (Variable.PropertyFlags & CPF_DisableEditOnInstance) == 0);
        VarObj->SetBoolField(TEXT("is_config"), (Variable.PropertyFlags & CPF_Config) != 0);

        // Replication
        VarObj->SetNumberField(TEXT("replication"), (int32)Variable.ReplicationCondition);

        VariableArray.Add(MakeShared<FJsonValueObject>(VarObj));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint_path"), BlueprintPath);
    
    if (bSpecificVariable)
    {
        ResultObj->SetStringField(TEXT("variable_name"), VariableName);
        if (VariableArray.Num() > 0)
        {
            ResultObj->SetObjectField(TEXT("variable"), VariableArray[0]->AsObject());
        }
        else
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Variable not found: %s"), *VariableName));
        }
    }
    else
    {
        ResultObj->SetArrayField(TEXT("variables"), VariableArray);
        ResultObj->SetNumberField(TEXT("variable_count"), VariableArray.Num());
    }

    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetBlueprintFunctionDetails(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
    }

    FString FunctionName;
    bool bSpecificFunction = Params->TryGetStringField(TEXT("function_name"), FunctionName);

    bool bIncludeGraph = true;
    Params->TryGetBoolField(TEXT("include_graph"), bIncludeGraph);

    // Load the blueprint
    UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
    }

    TArray<TSharedPtr<FJsonValue>> FunctionArray;

    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        if (!Graph) continue;

        // If looking for specific function, skip others
        if (bSpecificFunction && Graph->GetName() != FunctionName)
        {
            continue;
        }

        TSharedPtr<FJsonObject> FuncObj = MakeShared<FJsonObject>();
        FuncObj->SetStringField(TEXT("name"), Graph->GetName());
        FuncObj->SetStringField(TEXT("graph_type"), TEXT("Function"));

        // Get function signature from graph
        TArray<TSharedPtr<FJsonValue>> InputPins;
        TArray<TSharedPtr<FJsonValue>> OutputPins;

        // Find function entry and result nodes
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (Node)
            {
                if (Node->GetClass()->GetName().Contains(TEXT("FunctionEntry")))
                {
                    // Process input parameters
                    for (UEdGraphPin* Pin : Node->Pins)
                    {
                        if (Pin && Pin->Direction == EGPD_Output && Pin->PinName != TEXT("then"))
                        {
                            TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
                            PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
                            PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
                            InputPins.Add(MakeShared<FJsonValueObject>(PinObj));
                        }
                    }
                }
                else if (Node->GetClass()->GetName().Contains(TEXT("FunctionResult")))
                {
                    // Process output parameters
                    for (UEdGraphPin* Pin : Node->Pins)
                    {
                        if (Pin && Pin->Direction == EGPD_Input && Pin->PinName != TEXT("exec"))
                        {
                            TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
                            PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
                            PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
                            OutputPins.Add(MakeShared<FJsonValueObject>(PinObj));
                        }
                    }
                }
            }
        }

        FuncObj->SetArrayField(TEXT("input_parameters"), InputPins);
        FuncObj->SetArrayField(TEXT("output_parameters"), OutputPins);
        FuncObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());

        // Include graph details if requested
        if (bIncludeGraph)
        {
            TArray<TSharedPtr<FJsonValue>> NodeArray;
            for (UEdGraphNode* Node : Graph->Nodes)
            {
                if (Node)
                {
                    TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
                    NodeObj->SetStringField(TEXT("name"), Node->GetName());
                    NodeObj->SetStringField(TEXT("class"), Node->GetClass()->GetName());
                    NodeObj->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
                    NodeArray.Add(MakeShared<FJsonValueObject>(NodeObj));
                }
            }
            FuncObj->SetArrayField(TEXT("graph_nodes"), NodeArray);
        }

        FunctionArray.Add(MakeShared<FJsonValueObject>(FuncObj));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint_path"), BlueprintPath);
    
    if (bSpecificFunction)
    {
        ResultObj->SetStringField(TEXT("function_name"), FunctionName);
        if (FunctionArray.Num() > 0)
        {
            ResultObj->SetObjectField(TEXT("function"), FunctionArray[0]->AsObject());
        }
        else
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Function not found: %s"), *FunctionName));
        }
    }
    else
    {
        ResultObj->SetArrayField(TEXT("functions"), FunctionArray);
        ResultObj->SetNumberField(TEXT("function_count"), FunctionArray.Num());
    }

    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleFixDuplicateAnimSlots(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
    }

    UBlueprint* BP = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintPath);
    UAnimBlueprint* AnimBP = Cast<UAnimBlueprint>(BP);
    if (!AnimBP)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Asset '%s' is not an Animation Blueprint or could not be loaded"), *BlueprintPath));
    }

    // Collect all graphs in the animation blueprint (including nested sub-graphs)
    TArray<UEdGraph*> AllGraphs;
    AnimBP->GetAllGraphs(AllGraphs);

    // Track unique slot names; keep the first occurrence, remove subsequent duplicates
    TMap<FName, UAnimGraphNode_Slot*> SeenSlots;
    TArray<UEdGraphNode*> NodesToRemove;

    for (UEdGraph* Graph : AllGraphs)
    {
        if (!Graph) continue;

        for (UEdGraphNode* Node : Graph->Nodes)
        {
            UAnimGraphNode_Slot* SlotNode = Cast<UAnimGraphNode_Slot>(Node);
            if (!SlotNode) continue;

            FName SlotName = SlotNode->Node.SlotName;
            if (SeenSlots.Contains(SlotName))
            {
                NodesToRemove.Add(SlotNode);
            }
            else
            {
                SeenSlots.Add(SlotName, SlotNode);
            }
        }
    }

    if (NodesToRemove.IsEmpty())
    {
        TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
        Response->SetStringField(TEXT("status"), TEXT("no_duplicates_found"));
        Response->SetStringField(TEXT("message"), TEXT("No duplicate slot nodes found in the animation graph"));
        return Response;
    }

    TArray<TSharedPtr<FJsonValue>> RemovedSlotNames;
    for (UEdGraphNode* NodeToRemove : NodesToRemove)
    {
        UAnimGraphNode_Slot* SlotNode = CastChecked<UAnimGraphNode_Slot>(NodeToRemove);
        RemovedSlotNames.Add(MakeShared<FJsonValueString>(SlotNode->Node.SlotName.ToString()));
        FBlueprintEditorUtils::RemoveNode(AnimBP, NodeToRemove, /*bDontRecompile=*/true);
    }

    // Recompile the blueprint now that duplicates are removed
    FKismetEditorUtilities::CompileBlueprint(AnimBP);

    // Persist the change
    UEditorAssetLibrary::SaveAsset(BlueprintPath, /*bOnlyIfIsDirty=*/false);

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetNumberField(TEXT("removed_count"), NodesToRemove.Num());
    Response->SetArrayField(TEXT("removed_slots"), RemovedSlotNames);
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleListBlueprintGraphs(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
    }

    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintPath);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintPath));
    }

    // Gather every graph reachable from this blueprint
    TArray<UEdGraph*> AllGraphs;
    Blueprint->GetAllGraphs(AllGraphs);

    TArray<TSharedPtr<FJsonValue>> GraphArray;
    for (UEdGraph* Graph : AllGraphs)
    {
        if (!Graph) continue;

        TSharedPtr<FJsonObject> GraphObj = MakeShared<FJsonObject>();
        GraphObj->SetStringField(TEXT("name"), Graph->GetName());
        GraphObj->SetStringField(TEXT("type"), Graph->GetClass()->GetName());
        GraphObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());

        // Report any AnimGraphNode_Slot names so callers can detect duplicates
        TArray<TSharedPtr<FJsonValue>> SlotNames;
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (Node && Node->GetClass()->GetName().Contains(TEXT("Slot")))
            {
                TSharedPtr<FJsonObject> SlotObj = MakeShared<FJsonObject>();
                SlotObj->SetStringField(TEXT("node_name"), Node->GetName());
                SlotObj->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
                SlotNames.Add(MakeShared<FJsonValueObject>(SlotObj));
            }
        }
        if (SlotNames.Num() > 0)
        {
            GraphObj->SetArrayField(TEXT("slot_nodes"), SlotNames);
        }

        GraphArray.Add(MakeShared<FJsonValueObject>(GraphObj));
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("blueprint_path"), BlueprintPath);
    Response->SetNumberField(TEXT("graph_count"), AllGraphs.Num());
    Response->SetArrayField(TEXT("graphs"), GraphArray);
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetAnimationBlueprintSkeleton(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
    }

    FString SkeletonPath;
    if (!Params->TryGetStringField(TEXT("skeleton_path"), SkeletonPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'skeleton_path' parameter"));
    }

    UBlueprint* BP = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintPath);
    UAnimBlueprint* AnimBP = Cast<UAnimBlueprint>(BP);
    if (!AnimBP)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Asset '%s' is not an Animation Blueprint or could not be loaded"), *BlueprintPath));
    }

    // Load the target skeleton
    USkeleton* NewSkeleton = Cast<USkeleton>(StaticLoadObject(USkeleton::StaticClass(), nullptr, *SkeletonPath));
    if (!NewSkeleton)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Skeleton not found: %s"), *SkeletonPath));
    }

    // Get current skeleton for logging
    USkeleton* OldSkeleton = AnimBP->TargetSkeleton;
    FString OldSkeletonPath = OldSkeleton ? OldSkeleton->GetPathName() : TEXT("None");

    // Set the new skeleton
    AnimBP->TargetSkeleton = NewSkeleton;

    // Get all graphs and refresh their skeleton references
    TArray<UEdGraph*> AllGraphs;
    AnimBP->GetAllGraphs(AllGraphs);

    // Mark the blueprint as modified
    AnimBP->Modify();

    // Refresh all animation nodes to use the new skeleton
    for (UEdGraph* Graph : AllGraphs)
    {
        if (!Graph) continue;

        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (!Node) continue;

            // Refresh the node to pick up new skeleton
            Node->ReconstructNode();
            Node->Modify();
        }

        Graph->Modify();
    }

    // Recompile the blueprint with the new skeleton
    FKismetEditorUtilities::CompileBlueprint(AnimBP);

    // Save the asset
    UEditorAssetLibrary::SaveAsset(BlueprintPath, /*bOnlyIfIsDirty=*/false);

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("blueprint_path"), BlueprintPath);
    Response->SetStringField(TEXT("old_skeleton"), OldSkeletonPath);
    Response->SetStringField(TEXT("new_skeleton"), SkeletonPath);
    Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Animation blueprint retargeted from %s to %s"), *OldSkeletonPath, *SkeletonPath));

    if (OldSkeleton)
    {
        const TSet<FName> NewSkeletonBones = BuildBoneSet(NewSkeleton->GetReferenceSkeleton());
        TArray<TSharedPtr<FJsonValue>> MissingBones = BuildMissingBoneArray(OldSkeleton->GetReferenceSkeleton(), NewSkeletonBones);
        Response->SetArrayField(TEXT("missing_old_bones_in_new"), MissingBones);
        Response->SetNumberField(TEXT("missing_old_bones_in_new_count"), MissingBones.Num());
    }

    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleRetargetAnimationBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    FString AnimationBlueprintPath;
    if (!Params->TryGetStringField(TEXT("animation_blueprint_path"), AnimationBlueprintPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'animation_blueprint_path' parameter"));
    }

    FString NewSkeletonPath;
    if (!Params->TryGetStringField(TEXT("new_skeleton"), NewSkeletonPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'new_skeleton' parameter"));
    }

    FString ExpectedOldSkeletonPath;
    Params->TryGetStringField(TEXT("old_skeleton"), ExpectedOldSkeletonPath);

    if (!ExpectedOldSkeletonPath.IsEmpty())
    {
        UBlueprint* BP = FEpicUnrealMCPCommonUtils::FindBlueprint(AnimationBlueprintPath);
        UAnimBlueprint* AnimBP = Cast<UAnimBlueprint>(BP);
        if (!AnimBP)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Asset '%s' is not an Animation Blueprint or could not be loaded"), *AnimationBlueprintPath));
        }

        const FString CurrentOldSkeleton = AnimBP->TargetSkeleton ? AnimBP->TargetSkeleton->GetPathName() : TEXT("None");
        if (!CurrentOldSkeleton.Equals(ExpectedOldSkeletonPath, ESearchCase::IgnoreCase))
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Old skeleton mismatch. Expected '%s' but animation blueprint currently uses '%s'"),
                    *ExpectedOldSkeletonPath,
                    *CurrentOldSkeleton));
        }
    }

    TSharedPtr<FJsonObject> ForwardParams = MakeShared<FJsonObject>();
    ForwardParams->SetStringField(TEXT("blueprint_path"), AnimationBlueprintPath);
    ForwardParams->SetStringField(TEXT("skeleton_path"), NewSkeletonPath);
    return HandleSetAnimationBlueprintSkeleton(ForwardParams);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetSkeletonBones(const TSharedPtr<FJsonObject>& Params)
{
    FString SkeletonPath;
    if (!Params->TryGetStringField(TEXT("skeleton_path"), SkeletonPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'skeleton_path' parameter"));
    }

    USkeleton* Skeleton = Cast<USkeleton>(StaticLoadObject(USkeleton::StaticClass(), nullptr, *SkeletonPath));
    if (!Skeleton)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Skeleton not found: %s"), *SkeletonPath));
    }

    const FReferenceSkeleton& ReferenceSkeleton = Skeleton->GetReferenceSkeleton();
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("skeleton_path"), SkeletonPath);
    Response->SetNumberField(TEXT("bone_count"), ReferenceSkeleton.GetNum());
    Response->SetArrayField(TEXT("bones"), BuildBoneArray(ReferenceSkeleton));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleCompareSkeletonBones(const TSharedPtr<FJsonObject>& Params)
{
    FString SourceSkeletonPath;
    if (!Params->TryGetStringField(TEXT("source_skeleton_path"), SourceSkeletonPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_skeleton_path' parameter"));
    }

    FString TargetSkeletonPath;
    if (!Params->TryGetStringField(TEXT("target_skeleton_path"), TargetSkeletonPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'target_skeleton_path' parameter"));
    }

    USkeleton* SourceSkeleton = Cast<USkeleton>(StaticLoadObject(USkeleton::StaticClass(), nullptr, *SourceSkeletonPath));
    if (!SourceSkeleton)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Source skeleton not found: %s"), *SourceSkeletonPath));
    }

    USkeleton* TargetSkeleton = Cast<USkeleton>(StaticLoadObject(USkeleton::StaticClass(), nullptr, *TargetSkeletonPath));
    if (!TargetSkeleton)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Target skeleton not found: %s"), *TargetSkeletonPath));
    }

    const FReferenceSkeleton& SourceRefSkeleton = SourceSkeleton->GetReferenceSkeleton();
    const FReferenceSkeleton& TargetRefSkeleton = TargetSkeleton->GetReferenceSkeleton();
    const TSet<FName> SourceBones = BuildBoneSet(SourceRefSkeleton);
    const TSet<FName> TargetBones = BuildBoneSet(TargetRefSkeleton);

    TArray<TSharedPtr<FJsonValue>> MissingInTarget = BuildMissingBoneArray(SourceRefSkeleton, TargetBones);
    TArray<TSharedPtr<FJsonValue>> MissingInSource = BuildMissingBoneArray(TargetRefSkeleton, SourceBones);

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("source_skeleton_path"), SourceSkeletonPath);
    Response->SetStringField(TEXT("target_skeleton_path"), TargetSkeletonPath);
    Response->SetNumberField(TEXT("source_bone_count"), SourceRefSkeleton.GetNum());
    Response->SetNumberField(TEXT("target_bone_count"), TargetRefSkeleton.GetNum());
    Response->SetArrayField(TEXT("missing_in_target"), MissingInTarget);
    Response->SetArrayField(TEXT("missing_in_source"), MissingInSource);
    Response->SetNumberField(TEXT("missing_in_target_count"), MissingInTarget.Num());
    Response->SetNumberField(TEXT("missing_in_source_count"), MissingInSource.Num());
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetDataTable(const TSharedPtr<FJsonObject>& Params)
{
    FString DataTablePath;
    if (!Params->TryGetStringField(TEXT("datatable_path"), DataTablePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'datatable_path' parameter"));
    }

    // Load the DataTable
    UDataTable* DataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *DataTablePath));
    if (!DataTable)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("DataTable not found: %s"), *DataTablePath));
    }

    // Get the DataTable's row struct type
    const UScriptStruct* RowStruct = DataTable->GetRowStruct();
    if (!RowStruct)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Could not access DataTable row struct type"));
    }

    // Create response object
    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("datatable_path"), DataTablePath);

    // Get all row names
    TArray<FName> RowNames = DataTable->GetRowNames();
    
    // Create array of rows
    TArray<TSharedPtr<FJsonValue>> RowsArray;

    // Iterate through all rows and collect their data
    for (const FName& RowName : RowNames)
    {
        FTableRowBase* RowPtr = DataTable->FindRow<FTableRowBase>(RowName, TEXT("GetDataTable"));
        if (!RowPtr)
        {
            continue;
        }

        TSharedPtr<FJsonObject> RowObject = MakeShared<FJsonObject>();
        RowObject->SetStringField(TEXT("row_name"), RowName.ToString());

        // Get all properties from the struct
        for (TFieldIterator<FProperty> PropIt(RowStruct); PropIt; ++PropIt)
        {
            FProperty* Property = *PropIt;
            if (!Property)
            {
                continue;
            }

            void* PropertyAddress = Property->ContainerPtrToValuePtr<void>(RowPtr);
            if (!PropertyAddress)
            {
                continue;
            }

            // Handle different property types
            if (FClassProperty* ClassProp = CastField<FClassProperty>(Property))
            {
                UClass* ClassValue = *(UClass**)PropertyAddress;
                if (ClassValue)
                {
                    RowObject->SetStringField(*Property->GetName(), ClassValue->GetPathName());
                }
                else
                {
                    RowObject->SetStringField(*Property->GetName(), TEXT(""));
                }
            }
            else if (FSoftObjectProperty* SoftObjProp = CastField<FSoftObjectProperty>(Property))
            {
                FSoftObjectPath ObjPath = *(FSoftObjectPath*)PropertyAddress;
                RowObject->SetStringField(*Property->GetName(), ObjPath.ToString());
            }
            else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
            {
                FString StrValue = *(FString*)PropertyAddress;
                RowObject->SetStringField(*Property->GetName(), StrValue);
            }
            else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
            {
                FName NameValue = *(FName*)PropertyAddress;
                RowObject->SetStringField(*Property->GetName(), NameValue.ToString());
            }
            else if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
            {
                FText TextValue = *(FText*)PropertyAddress;
                RowObject->SetStringField(*Property->GetName(), TextValue.ToString());
            }
            else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
            {
                int32 IntValue = *(int32*)PropertyAddress;
                RowObject->SetNumberField(*Property->GetName(), IntValue);
            }
            else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
            {
                float FloatValue = *(float*)PropertyAddress;
                RowObject->SetNumberField(*Property->GetName(), FloatValue);
            }
            else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
            {
                bool BoolValue = BoolProp->GetPropertyValue(PropertyAddress);
                RowObject->SetBoolField(*Property->GetName(), BoolValue);
            }
        }

        RowsArray.Add(MakeShared<FJsonValueObject>(RowObject));
    }

    Response->SetArrayField(TEXT("rows"), RowsArray);
    Response->SetNumberField(TEXT("row_count"), RowNames.Num());
    Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Retrieved %d rows from DataTable"), RowNames.Num()));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleUpdateDataTableRow(const TSharedPtr<FJsonObject>& Params)
{
    FString DataTablePath;
    if (!Params->TryGetStringField(TEXT("datatable_path"), DataTablePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'datatable_path' parameter"));
    }

    FString RowName;
    if (!Params->TryGetStringField(TEXT("row_name"), RowName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'row_name' parameter"));
    }

    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    // Load the DataTable
    UDataTable* DataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *DataTablePath));
    if (!DataTable)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("DataTable not found: %s"), *DataTablePath));
    }

    // Get the row
    FTableRowBase* RowPtr = DataTable->FindRow<FTableRowBase>(FName(*RowName), TEXT("UpdateDataTableRow"));
    if (!RowPtr)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Row '%s' not found in DataTable '%s'"), *RowName, *DataTablePath));
    }

    // Get the DataTable's row struct type
    const UScriptStruct* RowStruct = DataTable->GetRowStruct();
    if (!RowStruct)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Could not access DataTable row struct type"));
    }

    // Find the property
    FProperty* Property = RowStruct->FindPropertyByName(FName(*PropertyName));
    if (!Property)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Property '%s' not found in DataTable row struct"), *PropertyName));
    }

    void* PropertyAddress = Property->ContainerPtrToValuePtr<void>(RowPtr);
    if (!PropertyAddress)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not access property '%s'"), *PropertyName));
    }

    // Handle different property types based on JSON value
    TSharedPtr<FJsonValue> ValueJson = Params->TryGetField(TEXT("value"));
    if (!ValueJson.IsValid())
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' parameter"));
    }

    // Set the property value based on its type
    if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
    {
        *(FString*)PropertyAddress = ValueJson->AsString();
    }
    else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
    {
        *(FName*)PropertyAddress = FName(*ValueJson->AsString());
    }
    else if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
    {
        *(FText*)PropertyAddress = FText::FromString(ValueJson->AsString());
    }
    else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
    {
        *(int32*)PropertyAddress = static_cast<int32>(ValueJson->AsNumber());
    }
    else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
    {
        *(float*)PropertyAddress = static_cast<float>(ValueJson->AsNumber());
    }
    else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
    {
        BoolProp->SetPropertyValue(PropertyAddress, ValueJson->AsBool());
    }
    else if (FClassProperty* ClassProp = CastField<FClassProperty>(Property))
    {
        FString ClassPath = ValueJson->AsString();
        if (!ClassPath.IsEmpty())
        {
            TSubclassOf<UObject> ClassValue = StaticLoadClass(UObject::StaticClass(), nullptr, *ClassPath);
            if (ClassValue)
            {
                *(TSubclassOf<UObject>*)PropertyAddress = ClassValue;
            }
            else
            {
                return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
                    FString::Printf(TEXT("Class not found: %s"), *ClassPath));
            }
        }
    }
    else if (FSoftObjectProperty* SoftObjProp = CastField<FSoftObjectProperty>(Property))
    {
        *(FSoftObjectPath*)PropertyAddress = FSoftObjectPath(ValueJson->AsString());
    }
    else
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Unsupported property type for '%s'"), *PropertyName));
    }

    // Mark the DataTable as modified and save
    DataTable->Modify();
    UEditorAssetLibrary::SaveAsset(DataTablePath, /*bOnlyIfIsDirty=*/false);

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("datatable_path"), DataTablePath);
    Response->SetStringField(TEXT("row_name"), RowName);
    Response->SetStringField(TEXT("property_name"), PropertyName);
    Response->SetField(TEXT("new_value"), ValueJson);
    Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Updated property '%s' in row '%s'"), *PropertyName, *RowName));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleCreateDataTable(const TSharedPtr<FJsonObject>& Params)
{
    FString DataTableName;
    if (!Params->TryGetStringField(TEXT("name"), DataTableName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    FString RowStructPath;
    if (!Params->TryGetStringField(TEXT("row_struct_path"), RowStructPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'row_struct_path' parameter"));
    }

    FString PackagePath = TEXT("/Game/DataTables/");
    if (Params->TryGetStringField(TEXT("package_path"), PackagePath))
    {
        // Use provided path, ensure it ends with /
        if (!PackagePath.EndsWith(TEXT("/")))
        {
            PackagePath += TEXT("/");
        }
    }

    // Check if DataTable already exists
    if (UEditorAssetLibrary::DoesAssetExist(PackagePath + DataTableName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("DataTable already exists: %s"), *(PackagePath + DataTableName)));
    }

    // Load the row struct
    UScriptStruct* RowStruct = LoadObject<UScriptStruct>(nullptr, *RowStructPath);
    if (!RowStruct)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Row struct not found: %s"), *RowStructPath));
    }

    // Create a new UDataTable using NewObject
    UDataTable* NewDataTable = NewObject<UDataTable>(GetTransientPackage(), *DataTableName);
    if (!NewDataTable)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create DataTable object"));
    }

    // Set the row struct
    NewDataTable->RowStruct = RowStruct;

    // Create the package and save the asset
    const FString AssetPath = PackagePath + DataTableName;
    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to create package: %s"), *PackagePath));
    }

    // Move the DataTable to the package
    NewDataTable->Rename(*DataTableName, Package, REN_NonTransactional);
    NewDataTable->MarkPackageDirty();

    // Save the package
    if (!UEditorAssetLibrary::SaveAsset(AssetPath, /*bOnlyIfIsDirty=*/false))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to save DataTable: %s"), *AssetPath));
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("datatable_path"), AssetPath);
    Response->SetStringField(TEXT("row_struct_path"), RowStructPath);
    Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Created new DataTable: %s"), *DataTableName));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleDeleteDataTableRow(const TSharedPtr<FJsonObject>& Params)
{
    FString DataTablePath;
    if (!Params->TryGetStringField(TEXT("datatable_path"), DataTablePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'datatable_path' parameter"));
    }

    FString RowName;
    if (!Params->TryGetStringField(TEXT("row_name"), RowName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'row_name' parameter"));
    }

    // Load the DataTable
    UDataTable* DataTable = LoadObject<UDataTable>(nullptr, *DataTablePath);
    if (!DataTable)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("DataTable not found: %s"), *DataTablePath));
    }

    // Check if row exists
    if (!DataTable->GetRowMap().Contains(FName(*RowName)))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Row '%s' not found in DataTable"), *RowName));
    }

    // Remove the row
    DataTable->RemoveRow(FName(*RowName));

    // Mark as modified and save
    DataTable->Modify();
    UEditorAssetLibrary::SaveAsset(DataTablePath, /*bOnlyIfIsDirty=*/false);

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("datatable_path"), DataTablePath);
    Response->SetStringField(TEXT("row_name"), RowName);
    Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Deleted row '%s' from DataTable"), *RowName));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleDeleteAsset(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
    }

    // Check if asset exists
    if (!UEditorAssetLibrary::DoesAssetExist(AssetPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Asset not found: %s"), *AssetPath));
    }

    // Delete the asset
    bool bSuccess = UEditorAssetLibrary::DeleteAsset(AssetPath);
    
    if (!bSuccess)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to delete asset: %s"), *AssetPath));
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("asset_path"), AssetPath);
    Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Successfully deleted asset: %s"), *AssetPath));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleAddDataTableRow(const TSharedPtr<FJsonObject>& Params)
{
    FString DataTablePath;
    if (!Params->TryGetStringField(TEXT("data_table_path"), DataTablePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'data_table_path' parameter"));
    }

    FString RowName;
    if (!Params->TryGetStringField(TEXT("row_name"), RowName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'row_name' parameter"));
    }

    const TSharedPtr<FJsonObject>* RowDataPtr;
    if (!Params->TryGetObjectField(TEXT("row_data"), RowDataPtr))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'row_data' parameter"));
    }

    // Load the DataTable
    UDataTable* DataTable = LoadObject<UDataTable>(nullptr, *DataTablePath);
    if (!DataTable)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load DataTable: %s"), *DataTablePath));
    }

    // Check if row already exists
    if (DataTable->FindRow<FTableRowBase>(FName(*RowName), TEXT("")))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Row '%s' already exists"), *RowName));
    }

    // Get row struct
    const UScriptStruct* RowStruct = DataTable->GetRowStruct();
    if (!RowStruct)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get row struct"));
    }

    // Create new row data
    uint8* NewRowData = (uint8*)FMemory::Malloc(RowStruct->GetStructureSize());
    RowStruct->InitializeStruct(NewRowData);

    // Populate the row from JSON
    if (!FJsonObjectConverter::JsonObjectToUStruct((*RowDataPtr).ToSharedRef(), RowStruct, NewRowData, 0, 0))
    {
        RowStruct->DestroyStruct(NewRowData);
        FMemory::Free(NewRowData);
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to convert JSON to row structure"));
    }

    // Add the row
    DataTable->Modify();
    DataTable->AddRow(FName(*RowName), *reinterpret_cast<FTableRowBase*>(NewRowData));

    // Cleanup
    RowStruct->DestroyStruct(NewRowData);
    FMemory::Free(NewRowData);

    // Save the asset
    UEditorAssetLibrary::SaveAsset(DataTablePath, false);

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("row_name"), RowName);
    Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Added row '%s' to DataTable"), *RowName));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleRenameDataTableRow(const TSharedPtr<FJsonObject>& Params)
{
    FString DataTablePath;
    if (!Params->TryGetStringField(TEXT("data_table_path"), DataTablePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'data_table_path' parameter"));
    }

    FString OldRowName;
    if (!Params->TryGetStringField(TEXT("old_row_name"), OldRowName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'old_row_name' parameter"));
    }

    FString NewRowName;
    if (!Params->TryGetStringField(TEXT("new_row_name"), NewRowName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'new_row_name' parameter"));
    }

    // Load the DataTable
    UDataTable* DataTable = LoadObject<UDataTable>(nullptr, *DataTablePath);
    if (!DataTable)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load DataTable: %s"), *DataTablePath));
    }

    // Check if old row exists
    FName OldKey(*OldRowName);
    if (!DataTable->FindRow<FTableRowBase>(OldKey, TEXT("")))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Row '%s' not found"), *OldRowName));
    }

    // Check if new row name already exists
    FName NewKey(*NewRowName);
    if (DataTable->FindRow<FTableRowBase>(NewKey, TEXT("")))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Row '%s' already exists"), *NewRowName));
    }

    // Manual rename implementation (RenameRow doesn't exist in UE5.7)
    uint8* RowData = DataTable->FindRowUnchecked(OldKey);
    if (!RowData)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to find row data"));
    }

    // Create a copy of the row data
    const UScriptStruct* RowStruct = DataTable->GetRowStruct();
    uint8* NewRowData = (uint8*)FMemory::Malloc(RowStruct->GetStructureSize());
    RowStruct->InitializeStruct(NewRowData);
    RowStruct->CopyScriptStruct(NewRowData, RowData);

    // Remove old row and add with new name
    DataTable->Modify();
    DataTable->RemoveRow(OldKey);
    DataTable->AddRow(NewKey, *reinterpret_cast<FTableRowBase*>(NewRowData));

    // Cleanup
    RowStruct->DestroyStruct(NewRowData);
    FMemory::Free(NewRowData);

    UEditorAssetLibrary::SaveAsset(DataTablePath, false);

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("old_row_name"), OldRowName);
    Response->SetStringField(TEXT("new_row_name"), NewRowName);
    Response->SetStringField(TEXT("message"), FString::Printf(TEXT("Renamed row from '%s' to '%s'"), *OldRowName, *NewRowName));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetDataTableRowNames(const TSharedPtr<FJsonObject>& Params)
{
    FString DataTablePath;
    if (!Params->TryGetStringField(TEXT("data_table_path"), DataTablePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'data_table_path' parameter"));
    }

    // Load the DataTable
    UDataTable* DataTable = LoadObject<UDataTable>(nullptr, *DataTablePath);
    if (!DataTable)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load DataTable: %s"), *DataTablePath));
    }

    // Get all row names
    TArray<FName> RowNames = DataTable->GetRowNames();

    // Convert to JSON array
    TArray<TSharedPtr<FJsonValue>> RowNameArray;
    for (const FName& RowName : RowNames)
    {
        RowNameArray.Add(MakeShared<FJsonValueString>(RowName.ToString()));
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetArrayField(TEXT("row_names"), RowNameArray);
    Response->SetNumberField(TEXT("count"), RowNames.Num());
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleRenameAsset(const TSharedPtr<FJsonObject>& Params)
{
    FString SourcePath;
    if (!Params->TryGetStringField(TEXT("source_path"), SourcePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_path' parameter"));
    }

    FString DestinationPath;
    if (!Params->TryGetStringField(TEXT("destination_path"), DestinationPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'destination_path' parameter"));
    }

    if (!UEditorAssetLibrary::DoesAssetExist(SourcePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Source asset not found: %s"), *SourcePath));
    }

    bool bSuccess = UEditorAssetLibrary::RenameAsset(SourcePath, DestinationPath);
    
    if (!bSuccess)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to rename asset from %s to %s"), *SourcePath, *DestinationPath));
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("old_path"), SourcePath);
    Response->SetStringField(TEXT("new_path"), DestinationPath);
    Response->SetStringField(TEXT("message"), TEXT("Asset renamed successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleDuplicateAsset(const TSharedPtr<FJsonObject>& Params)
{
    FString SourcePath;
    if (!Params->TryGetStringField(TEXT("source_path"), SourcePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_path' parameter"));
    }

    FString DestinationPath;
    if (!Params->TryGetStringField(TEXT("destination_path"), DestinationPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'destination_path' parameter"));
    }

    if (!UEditorAssetLibrary::DoesAssetExist(SourcePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Source asset not found: %s"), *SourcePath));
    }

    UObject* DuplicatedAsset = UEditorAssetLibrary::DuplicateAsset(SourcePath, DestinationPath);
    
    if (!DuplicatedAsset)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to duplicate asset from %s to %s"), *SourcePath, *DestinationPath));
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("source_path"), SourcePath);
    Response->SetStringField(TEXT("destination_path"), DestinationPath);
    Response->SetStringField(TEXT("message"), TEXT("Asset duplicated successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleMoveAsset(const TSharedPtr<FJsonObject>& Params)
{
    FString SourcePath;
    if (!Params->TryGetStringField(TEXT("source_path"), SourcePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_path' parameter"));
    }

    FString DestinationPath;
    if (!Params->TryGetStringField(TEXT("destination_path"), DestinationPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'destination_path' parameter"));
    }

    if (!UEditorAssetLibrary::DoesAssetExist(SourcePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Source asset not found: %s"), *SourcePath));
    }

    bool bSuccess = UEditorAssetLibrary::RenameAsset(SourcePath, DestinationPath);
    
    if (!bSuccess)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to move asset from %s to %s"), *SourcePath, *DestinationPath));
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("source_path"), SourcePath);
    Response->SetStringField(TEXT("destination_path"), DestinationPath);
    Response->SetStringField(TEXT("message"), TEXT("Asset moved successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetAssetMetadata(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
    }

    if (!UEditorAssetLibrary::DoesAssetExist(AssetPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Asset not found: %s"), *AssetPath));
    }

    UObject* Asset = UEditorAssetLibrary::LoadAsset(AssetPath);
    if (!Asset)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load asset: %s"), *AssetPath));
    }

    TSharedPtr<FJsonObject> MetadataObj = MakeShared<FJsonObject>();
    MetadataObj->SetStringField(TEXT("name"), Asset->GetName());
    MetadataObj->SetStringField(TEXT("class"), Asset->GetClass()->GetName());
    MetadataObj->SetStringField(TEXT("path"), AssetPath);
    MetadataObj->SetStringField(TEXT("package"), Asset->GetPackage()->GetName());

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetObjectField(TEXT("metadata"), MetadataObj);
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleListAssetsInFolder(const TSharedPtr<FJsonObject>& Params)
{
    FString FolderPath;
    if (!Params->TryGetStringField(TEXT("folder_path"), FolderPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'folder_path' parameter"));
    }

    bool bRecursive = false;
    Params->TryGetBoolField(TEXT("recursive"), bRecursive);

    TArray<FString> AssetPaths = UEditorAssetLibrary::ListAssets(FolderPath, bRecursive);

    TArray<TSharedPtr<FJsonValue>> AssetArray;
    for (const FString& Path : AssetPaths)
    {
        AssetArray.Add(MakeShared<FJsonValueString>(Path));
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("folder_path"), FolderPath);
    Response->SetArrayField(TEXT("assets"), AssetArray);
    Response->SetNumberField(TEXT("count"), AssetPaths.Num());
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSaveAsset(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
    }

    if (!UEditorAssetLibrary::DoesAssetExist(AssetPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Asset not found: %s"), *AssetPath));
    }

    bool bSuccess = UEditorAssetLibrary::SaveAsset(AssetPath, false);
    
    if (!bSuccess)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to save asset: %s"), *AssetPath));
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("asset_path"), AssetPath);
    Response->SetStringField(TEXT("message"), TEXT("Asset saved successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetObjectProperty(const TSharedPtr<FJsonObject>& Params)
{
    FString ObjectPath;
    if (!Params->TryGetStringField(TEXT("object_path"), ObjectPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'object_path' parameter"));
    }

    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    UObject* Object = LoadObject<UObject>(nullptr, *ObjectPath);
    if (!Object)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load object: %s"), *ObjectPath));
    }

    FProperty* Property = Object->GetClass()->FindPropertyByName(FName(*PropertyName));
    if (!Property)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Property '%s' not found"), *PropertyName));
    }

    FString ValueString;
    const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Object);
    Property->ExportTextItem_Direct(ValueString, ValuePtr, nullptr, nullptr, PPF_None);

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("property_name"), PropertyName);
    Response->SetStringField(TEXT("value"), ValueString);
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetObjectProperty(const TSharedPtr<FJsonObject>& Params)
{
    FString ObjectPath;
    if (!Params->TryGetStringField(TEXT("object_path"), ObjectPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'object_path' parameter"));
    }

    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    FString Value;
    if (!Params->TryGetStringField(TEXT("value"), Value))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' parameter"));
    }

    UObject* Object = LoadObject<UObject>(nullptr, *ObjectPath);
    if (!Object)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load object: %s"), *ObjectPath));
    }

    FProperty* Property = Object->GetClass()->FindPropertyByName(FName(*PropertyName));
    if (!Property)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Property '%s' not found"), *PropertyName));
    }

    Object->Modify();
    void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Object);
    Property->ImportText_Direct(*Value, ValuePtr, nullptr, PPF_None);

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("property_name"), PropertyName);
    Response->SetStringField(TEXT("message"), TEXT("Property set successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetObjectProperties(const TSharedPtr<FJsonObject>& Params)
{
    FString ObjectPath;
    if (!Params->TryGetStringField(TEXT("object_path"), ObjectPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'object_path' parameter"));
    }

    UObject* Object = LoadObject<UObject>(nullptr, *ObjectPath);
    if (!Object)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load object: %s"), *ObjectPath));
    }

    TSharedPtr<FJsonObject> PropertiesObj = MakeShared<FJsonObject>();
    
    for (TFieldIterator<FProperty> PropIt(Object->GetClass()); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        FString PropertyName = Property->GetName();
        
        FString ValueString;
        const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Object);
        Property->ExportTextItem_Direct(ValueString, ValuePtr, nullptr, nullptr, PPF_None);
        
        PropertiesObj->SetStringField(PropertyName, ValueString);
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetObjectField(TEXT("properties"), PropertiesObj);
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetBlueprintComponents(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
    }

    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
    }

    TArray<TSharedPtr<FJsonValue>> ComponentArray;
    
    if (Blueprint->SimpleConstructionScript)
    {
        const TArray<USCS_Node*>& Nodes = Blueprint->SimpleConstructionScript->GetAllNodes();
        for (USCS_Node* Node : Nodes)
        {
            if (Node && Node->ComponentTemplate)
            {
                TSharedPtr<FJsonObject> ComponentObj = MakeShared<FJsonObject>();
                ComponentObj->SetStringField(TEXT("name"), Node->GetVariableName().ToString());
                ComponentObj->SetStringField(TEXT("class"), Node->ComponentClass->GetName());
                ComponentArray.Add(MakeShared<FJsonValueObject>(ComponentObj));
            }
        }
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetArrayField(TEXT("components"), ComponentArray);
    Response->SetNumberField(TEXT("count"), ComponentArray.Num());
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleRemoveComponentFromBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
    }

    if (!Blueprint->SimpleConstructionScript)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint has no construction script"));
    }

    USCS_Node* NodeToRemove = Blueprint->SimpleConstructionScript->FindSCSNode(FName(*ComponentName));
    if (!NodeToRemove)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Component '%s' not found"), *ComponentName));
    }

    Blueprint->Modify();
    Blueprint->SimpleConstructionScript->RemoveNode(NodeToRemove);
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("component_name"), ComponentName);
    Response->SetStringField(TEXT("message"), TEXT("Component removed successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetComponentProperties(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    const TSharedPtr<FJsonObject>* PropertiesPtr;
    if (!Params->TryGetObjectField(TEXT("properties"), PropertiesPtr))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'properties' parameter"));
    }

    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
    }

    if (!Blueprint->SimpleConstructionScript)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint has no construction script"));
    }

    USCS_Node* Node = Blueprint->SimpleConstructionScript->FindSCSNode(FName(*ComponentName));
    if (!Node || !Node->ComponentTemplate)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Component '%s' not found"), *ComponentName));
    }

    UObject* Component = Node->ComponentTemplate;
    Component->Modify();

    for (const auto& Pair : (*PropertiesPtr)->Values)
    {
        FString PropertyName = Pair.Key;
        FString Value = Pair.Value->AsString();
        
        FProperty* Property = Component->GetClass()->FindPropertyByName(FName(*PropertyName));
        if (Property)
        {
            void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Component);
            Property->ImportText_Direct(*Value, ValuePtr, nullptr, PPF_None);
        }
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("component_name"), ComponentName);
    Response->SetStringField(TEXT("message"), TEXT("Component properties set successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetBlueprintParentClass(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
    }

    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
    }

    FString ParentClassName = TEXT("None");
    if (Blueprint->ParentClass)
    {
        ParentClassName = Blueprint->ParentClass->GetName();
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("parent_class"), ParentClassName);
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetActorComponents(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No active world"));
    }

    AActor* Actor = nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->GetName() == ActorName)
        {
            Actor = *It;
            break;
        }
    }

    if (!Actor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Actor '%s' not found"), *ActorName));
    }

    TArray<TSharedPtr<FJsonValue>> ComponentArray;
    TArray<UActorComponent*> Components;
    Actor->GetComponents(Components);

    for (UActorComponent* Component : Components)
    {
        if (Component)
        {
            TSharedPtr<FJsonObject> ComponentObj = MakeShared<FJsonObject>();
            ComponentObj->SetStringField(TEXT("name"), Component->GetName());
            ComponentObj->SetStringField(TEXT("class"), Component->GetClass()->GetName());
            ComponentArray.Add(MakeShared<FJsonValueObject>(ComponentObj));
        }
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetArrayField(TEXT("components"), ComponentArray);
    Response->SetNumberField(TEXT("count"), ComponentArray.Num());
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleAddComponentToActor(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    FString ComponentClassName;
    if (!Params->TryGetStringField(TEXT("component_class"), ComponentClassName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_class' parameter"));
    }

    FString ComponentName;
    Params->TryGetStringField(TEXT("component_name"), ComponentName);

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No active world"));
    }

    AActor* Actor = nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->GetName() == ActorName)
        {
            Actor = *It;
            break;
        }
    }

    if (!Actor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Actor '%s' not found"), *ActorName));
    }

    UClass* ComponentClass = FindObject<UClass>(nullptr, *ComponentClassName);
    if (!ComponentClass || !ComponentClass->IsChildOf(UActorComponent::StaticClass()))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Invalid component class: %s"), *ComponentClassName));
    }

    Actor->Modify();
    UActorComponent* NewComponent = NewObject<UActorComponent>(Actor, ComponentClass, 
        ComponentName.IsEmpty() ? NAME_None : FName(*ComponentName));
    
    if (!NewComponent)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create component"));
    }

    NewComponent->RegisterComponent();
    Actor->AddInstanceComponent(NewComponent);

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("component_name"), NewComponent->GetName());
    Response->SetStringField(TEXT("message"), TEXT("Component added successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleRemoveComponentFromActor(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No active world"));
    }

    AActor* Actor = nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->GetName() == ActorName)
        {
            Actor = *It;
            break;
        }
    }

    if (!Actor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Actor '%s' not found"), *ActorName));
    }

    UActorComponent* ComponentToRemove = Actor->FindComponentByClass(UActorComponent::StaticClass());
    TArray<UActorComponent*> Components;
    Actor->GetComponents(Components);
    
    for (UActorComponent* Component : Components)
    {
        if (Component && Component->GetName() == ComponentName)
        {
            ComponentToRemove = Component;
            break;
        }
    }

    if (!ComponentToRemove)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Component '%s' not found"), *ComponentName));
    }

    Actor->Modify();
    ComponentToRemove->DestroyComponent();

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("component_name"), ComponentName);
    Response->SetStringField(TEXT("message"), TEXT("Component removed successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetComponentProperty(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No active world"));
    }

    AActor* Actor = nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->GetName() == ActorName)
        {
            Actor = *It;
            break;
        }
    }

    if (!Actor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Actor '%s' not found"), *ActorName));
    }

    UActorComponent* Component = nullptr;
    TArray<UActorComponent*> Components;
    Actor->GetComponents(Components);
    
    for (UActorComponent* Comp : Components)
    {
        if (Comp && Comp->GetName() == ComponentName)
        {
            Component = Comp;
            break;
        }
    }

    if (!Component)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Component '%s' not found"), *ComponentName));
    }

    FProperty* Property = Component->GetClass()->FindPropertyByName(FName(*PropertyName));
    if (!Property)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Property '%s' not found"), *PropertyName));
    }

    FString ValueString;
    const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Component);
    Property->ExportTextItem_Direct(ValueString, ValuePtr, nullptr, nullptr, PPF_None);

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("property_name"), PropertyName);
    Response->SetStringField(TEXT("value"), ValueString);
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetComponentProperty(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    FString Value;
    if (!Params->TryGetStringField(TEXT("value"), Value))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' parameter"));
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No active world"));
    }

    AActor* Actor = nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->GetName() == ActorName)
        {
            Actor = *It;
            break;
        }
    }

    if (!Actor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Actor '%s' not found"), *ActorName));
    }

    UActorComponent* Component = nullptr;
    TArray<UActorComponent*> Components;
    Actor->GetComponents(Components);
    
    for (UActorComponent* Comp : Components)
    {
        if (Comp && Comp->GetName() == ComponentName)
        {
            Component = Comp;
            break;
        }
    }

    if (!Component)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Component '%s' not found"), *ComponentName));
    }

    FProperty* Property = Component->GetClass()->FindPropertyByName(FName(*PropertyName));
    if (!Property)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Property '%s' not found"), *PropertyName));
    }

    Component->Modify();
    void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Component);
    Property->ImportText_Direct(*Value, ValuePtr, nullptr, PPF_None);

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("property_name"), PropertyName);
    Response->SetStringField(TEXT("message"), TEXT("Property set successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleCreateMaterialInstance(const TSharedPtr<FJsonObject>& Params)
{
    FString ParentMaterialPath;
    if (!Params->TryGetStringField(TEXT("parent_material_path"), ParentMaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'parent_material_path' parameter"));
    }

    FString InstancePath;
    if (!Params->TryGetStringField(TEXT("instance_path"), InstancePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'instance_path' parameter"));
    }

    UMaterial* ParentMaterial = LoadObject<UMaterial>(nullptr, *ParentMaterialPath);
    if (!ParentMaterial)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load parent material: %s"), *ParentMaterialPath));
    }

    FString PackagePath = FPackageName::GetLongPackagePath(InstancePath);
    FString AssetName = FPackageName::GetShortName(InstancePath);

    UPackage* Package = CreatePackage(*InstancePath);
    UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
    Factory->InitialParent = ParentMaterial;

    UMaterialInstanceConstant* MaterialInstance = Cast<UMaterialInstanceConstant>(
        Factory->FactoryCreateNew(UMaterialInstanceConstant::StaticClass(), Package, 
        FName(*AssetName), RF_Public | RF_Standalone, nullptr, GWarn));

    if (!MaterialInstance)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create material instance"));
    }

    FAssetRegistryModule::AssetCreated(MaterialInstance);
    MaterialInstance->MarkPackageDirty();
    Package->SetDirtyFlag(true);

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("instance_path"), InstancePath);
    Response->SetStringField(TEXT("message"), TEXT("Material instance created successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetMaterialScalarParameter(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    FString ParameterName;
    if (!Params->TryGetStringField(TEXT("parameter_name"), ParameterName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'parameter_name' parameter"));
    }

    double Value = 0.0;
    if (!Params->TryGetNumberField(TEXT("value"), Value))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' parameter"));
    }

    UMaterialInstanceConstant* MaterialInstance = LoadObject<UMaterialInstanceConstant>(nullptr, *MaterialPath);
    if (!MaterialInstance)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load material instance: %s"), *MaterialPath));
    }

    MaterialInstance->SetScalarParameterValueEditorOnly(FName(*ParameterName), static_cast<float>(Value));
    MaterialInstance->MarkPackageDirty();

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("parameter_name"), ParameterName);
    Response->SetNumberField(TEXT("value"), Value);
    Response->SetStringField(TEXT("message"), TEXT("Scalar parameter set successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetMaterialVectorParameter(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    FString ParameterName;
    if (!Params->TryGetStringField(TEXT("parameter_name"), ParameterName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'parameter_name' parameter"));
    }

    const TArray<TSharedPtr<FJsonValue>>* ValueArray;
    if (!Params->TryGetArrayField(TEXT("value"), ValueArray) || ValueArray->Num() < 3)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing or invalid 'value' parameter (expected array of 3-4 numbers)"));
    }

    UMaterialInstanceConstant* MaterialInstance = LoadObject<UMaterialInstanceConstant>(nullptr, *MaterialPath);
    if (!MaterialInstance)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load material instance: %s"), *MaterialPath));
    }

    FLinearColor Color(
        (*ValueArray)[0]->AsNumber(),
        (*ValueArray)[1]->AsNumber(),
        (*ValueArray)[2]->AsNumber(),
        ValueArray->Num() > 3 ? (*ValueArray)[3]->AsNumber() : 1.0f
    );

    MaterialInstance->SetVectorParameterValueEditorOnly(FName(*ParameterName), Color);
    MaterialInstance->MarkPackageDirty();

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("parameter_name"), ParameterName);
    Response->SetStringField(TEXT("message"), TEXT("Vector parameter set successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetMaterialTextureParameter(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    FString ParameterName;
    if (!Params->TryGetStringField(TEXT("parameter_name"), ParameterName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'parameter_name' parameter"));
    }

    FString TexturePath;
    if (!Params->TryGetStringField(TEXT("texture_path"), TexturePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'texture_path' parameter"));
    }

    UMaterialInstanceConstant* MaterialInstance = LoadObject<UMaterialInstanceConstant>(nullptr, *MaterialPath);
    if (!MaterialInstance)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load material instance: %s"), *MaterialPath));
    }

    UTexture* Texture = LoadObject<UTexture>(nullptr, *TexturePath);
    if (!Texture)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load texture: %s"), *TexturePath));
    }

    MaterialInstance->SetTextureParameterValueEditorOnly(FName(*ParameterName), Texture);
    MaterialInstance->MarkPackageDirty();

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("parameter_name"), ParameterName);
    Response->SetStringField(TEXT("texture_path"), TexturePath);
    Response->SetStringField(TEXT("message"), TEXT("Texture parameter set successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetMaterialParameters(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    UMaterialInstanceConstant* MaterialInstance = LoadObject<UMaterialInstanceConstant>(nullptr, *MaterialPath);
    if (!MaterialInstance)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load material instance: %s"), *MaterialPath));
    }

    TArray<TSharedPtr<FJsonValue>> ScalarParams;
    TArray<TSharedPtr<FJsonValue>> VectorParams;
    TArray<TSharedPtr<FJsonValue>> TextureParams;

    // Gather scalar parameters
    TArray<FMaterialParameterInfo> ScalarParameterInfo;
    TArray<FGuid> ScalarParameterGuids;
    MaterialInstance->GetAllScalarParameterInfo(ScalarParameterInfo, ScalarParameterGuids);
    
    for (const FMaterialParameterInfo& ParamInfo : ScalarParameterInfo)
    {
        float Value = 0.0f;
        if (MaterialInstance->GetScalarParameterValue(ParamInfo, Value))
        {
            TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();
            ParamObj->SetStringField(TEXT("name"), ParamInfo.Name.ToString());
            ParamObj->SetNumberField(TEXT("value"), Value);
            ScalarParams.Add(MakeShared<FJsonValueObject>(ParamObj));
        }
    }

    // Gather vector parameters
    TArray<FMaterialParameterInfo> VectorParameterInfo;
    TArray<FGuid> VectorParameterGuids;
    MaterialInstance->GetAllVectorParameterInfo(VectorParameterInfo, VectorParameterGuids);
    
    for (const FMaterialParameterInfo& ParamInfo : VectorParameterInfo)
    {
        FLinearColor Value;
        if (MaterialInstance->GetVectorParameterValue(ParamInfo, Value))
        {
            TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();
            ParamObj->SetStringField(TEXT("name"), ParamInfo.Name.ToString());
            
            TArray<TSharedPtr<FJsonValue>> ColorArray;
            ColorArray.Add(MakeShared<FJsonValueNumber>(Value.R));
            ColorArray.Add(MakeShared<FJsonValueNumber>(Value.G));
            ColorArray.Add(MakeShared<FJsonValueNumber>(Value.B));
            ColorArray.Add(MakeShared<FJsonValueNumber>(Value.A));
            ParamObj->SetArrayField(TEXT("value"), ColorArray);
            
            VectorParams.Add(MakeShared<FJsonValueObject>(ParamObj));
        }
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetArrayField(TEXT("scalar_parameters"), ScalarParams);
    Response->SetArrayField(TEXT("vector_parameters"), VectorParams);
    Response->SetArrayField(TEXT("texture_parameters"), TextureParams);
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSaveCurrentLevel(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No active world"));
    }

    ULevel* Level = World->GetCurrentLevel();
    if (!Level)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No current level"));
    }

    FString LevelPath = Level->GetOutermost()->GetName();
    bool bSuccess = UEditorAssetLibrary::SaveAsset(LevelPath, false);

    if (!bSuccess)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to save level"));
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("level_path"), LevelPath);
    Response->SetStringField(TEXT("message"), TEXT("Level saved successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleLoadLevel(const TSharedPtr<FJsonObject>& Params)
{
    FString LevelPath;
    if (!Params->TryGetStringField(TEXT("level_path"), LevelPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'level_path' parameter"));
    }

    bool bSuccess = UEditorAssetLibrary::DoesAssetExist(LevelPath);
    if (!bSuccess)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Level not found: %s"), *LevelPath));
    }

    UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(LevelPath);
    if (!LoadedAsset)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load level: %s"), *LevelPath));
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("level_path"), LevelPath);
    Response->SetStringField(TEXT("message"), TEXT("Level loaded successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetCurrentLevelName(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No active world"));
    }

    ULevel* Level = World->GetCurrentLevel();
    if (!Level)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No current level"));
    }

    FString LevelName = Level->GetOutermost()->GetName();

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("level_name"), LevelName);
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetAllLevels(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No active world"));
    }

    TArray<TSharedPtr<FJsonValue>> LevelArray;
    
    for (ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
    {
        if (StreamingLevel)
        {
            TSharedPtr<FJsonObject> LevelObj = MakeShared<FJsonObject>();
            LevelObj->SetStringField(TEXT("name"), StreamingLevel->GetWorldAssetPackageName());
            LevelObj->SetBoolField(TEXT("is_loaded"), StreamingLevel->IsLevelLoaded());
            LevelObj->SetBoolField(TEXT("is_visible"), StreamingLevel->IsLevelVisible());
            LevelArray.Add(MakeShared<FJsonValueObject>(LevelObj));
        }
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetArrayField(TEXT("levels"), LevelArray);
    Response->SetNumberField(TEXT("count"), LevelArray.Num());
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSpawnLight(const TSharedPtr<FJsonObject>& Params)
{
    FString LightType;
    if (!Params->TryGetStringField(TEXT("light_type"), LightType))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'light_type' parameter"));
    }

    const TArray<TSharedPtr<FJsonValue>>* LocationArray;
    if (!Params->TryGetArrayField(TEXT("location"), LocationArray) || LocationArray->Num() < 3)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing or invalid 'location' parameter (expected array of 3 numbers)"));
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No active world"));
    }

    FVector Location(
        (*LocationArray)[0]->AsNumber(),
        (*LocationArray)[1]->AsNumber(),
        (*LocationArray)[2]->AsNumber()
    );

    ALight* SpawnedLight = nullptr;
    
    if (LightType == TEXT("Point"))
    {
        SpawnedLight = World->SpawnActor<APointLight>(APointLight::StaticClass(), Location, FRotator::ZeroRotator);
    }
    else if (LightType == TEXT("Spot"))
    {
        SpawnedLight = World->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), Location, FRotator::ZeroRotator);
    }
    else if (LightType == TEXT("Directional"))
    {
        SpawnedLight = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), Location, FRotator::ZeroRotator);
    }
    else
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Invalid light type: %s. Valid types are: Point, Spot, Directional"), *LightType));
    }

    if (!SpawnedLight)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to spawn light"));
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("light_name"), SpawnedLight->GetName());
    Response->SetStringField(TEXT("light_type"), LightType);
    Response->SetStringField(TEXT("message"), TEXT("Light spawned successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetLightProperties(const TSharedPtr<FJsonObject>& Params)
{
    FString LightName;
    if (!Params->TryGetStringField(TEXT("light_name"), LightName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'light_name' parameter"));
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No active world"));
    }

    ALight* Light = nullptr;
    for (TActorIterator<ALight> It(World); It; ++It)
    {
        if (It->GetName() == LightName)
        {
            Light = *It;
            break;
        }
    }

    if (!Light)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Light '%s' not found"), *LightName));
    }

    ULightComponent* LightComponent = Light->GetLightComponent();
    if (!LightComponent)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Light has no light component"));
    }

    Light->Modify();

    double Intensity;
    if (Params->TryGetNumberField(TEXT("intensity"), Intensity))
    {
        LightComponent->SetIntensity(static_cast<float>(Intensity));
    }

    const TArray<TSharedPtr<FJsonValue>>* ColorArray;
    if (Params->TryGetArrayField(TEXT("color"), ColorArray) && ColorArray->Num() >= 3)
    {
        FLinearColor Color(
            (*ColorArray)[0]->AsNumber(),
            (*ColorArray)[1]->AsNumber(),
            (*ColorArray)[2]->AsNumber()
        );
        LightComponent->SetLightColor(Color);
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("light_name"), LightName);
    Response->SetStringField(TEXT("message"), TEXT("Light properties set successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSpawnCamera(const TSharedPtr<FJsonObject>& Params)
{
    const TArray<TSharedPtr<FJsonValue>>* LocationArray;
    if (!Params->TryGetArrayField(TEXT("location"), LocationArray) || LocationArray->Num() < 3)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing or invalid 'location' parameter (expected array of 3 numbers)"));
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No active world"));
    }

    FVector Location(
        (*LocationArray)[0]->AsNumber(),
        (*LocationArray)[1]->AsNumber(),
        (*LocationArray)[2]->AsNumber()
    );

    FRotator Rotation = FRotator::ZeroRotator;
    const TArray<TSharedPtr<FJsonValue>>* RotationArray;
    if (Params->TryGetArrayField(TEXT("rotation"), RotationArray) && RotationArray->Num() >= 3)
    {
        Rotation = FRotator(
            (*RotationArray)[0]->AsNumber(),
            (*RotationArray)[1]->AsNumber(),
            (*RotationArray)[2]->AsNumber()
        );
    }

    ACameraActor* Camera = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), Location, Rotation);
    
    if (!Camera)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to spawn camera"));
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("camera_name"), Camera->GetName());
    Response->SetStringField(TEXT("message"), TEXT("Camera spawned successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetCameraProperties(const TSharedPtr<FJsonObject>& Params)
{
    FString CameraName;
    if (!Params->TryGetStringField(TEXT("camera_name"), CameraName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'camera_name' parameter"));
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No active world"));
    }

    ACameraActor* Camera = nullptr;
    for (TActorIterator<ACameraActor> It(World); It; ++It)
    {
        if (It->GetName() == CameraName)
        {
            Camera = *It;
            break;
        }
    }

    if (!Camera)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Camera '%s' not found"), *CameraName));
    }

    UCameraComponent* CameraComponent = Camera->GetCameraComponent();
    if (!CameraComponent)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Camera has no camera component"));
    }

    Camera->Modify();

    double FOV;
    if (Params->TryGetNumberField(TEXT("fov"), FOV))
    {
        CameraComponent->SetFieldOfView(static_cast<float>(FOV));
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("camera_name"), CameraName);
    Response->SetStringField(TEXT("message"), TEXT("Camera properties set successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetActorCollisionPreset(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    FString PresetName;
    if (!Params->TryGetStringField(TEXT("preset_name"), PresetName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'preset_name' parameter"));
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No active world"));
    }

    AActor* Actor = nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->GetName() == ActorName)
        {
            Actor = *It;
            break;
        }
    }

    if (!Actor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Actor '%s' not found"), *ActorName));
    }

    UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
    if (!PrimitiveComp)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Actor has no primitive component"));
    }

    Actor->Modify();
    PrimitiveComp->SetCollisionProfileName(FName(*PresetName));

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("actor_name"), ActorName);
    Response->SetStringField(TEXT("preset_name"), PresetName);
    Response->SetStringField(TEXT("message"), TEXT("Collision preset set successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetCollisionEnabled(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    bool bEnabled = true;
    if (!Params->TryGetBoolField(TEXT("enabled"), bEnabled))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'enabled' parameter"));
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No active world"));
    }

    AActor* Actor = nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->GetName() == ActorName)
        {
            Actor = *It;
            break;
        }
    }

    if (!Actor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Actor '%s' not found"), *ActorName));
    }

    UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
    if (!PrimitiveComp)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Actor has no primitive component"));
    }

    Actor->Modify();
    PrimitiveComp->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("actor_name"), ActorName);
    Response->SetBoolField(TEXT("enabled"), bEnabled);
    Response->SetStringField(TEXT("message"), TEXT("Collision enabled state set successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleImportAsset(const TSharedPtr<FJsonObject>& Params)
{
    FString SourceFile;
    if (!Params->TryGetStringField(TEXT("source_file"), SourceFile))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_file' parameter"));
    }

    FString DestinationPath;
    if (!Params->TryGetStringField(TEXT("destination_path"), DestinationPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'destination_path' parameter"));
    }

    if (!FPaths::FileExists(SourceFile))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Source file not found: %s"), *SourceFile));
    }

    FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

    TArray<FString> FilesToImport;
    FilesToImport.Add(SourceFile);
    TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssets(FilesToImport, DestinationPath);

    TArray<FString> ImportedAssetPaths;
    for (UObject* Asset : ImportedAssets)
    {
        if (Asset)
        {
            ImportedAssetPaths.Add(Asset->GetPathName());
        }
    }

    if (ImportedAssetPaths.Num() == 0)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to import asset"));
    }

    TArray<TSharedPtr<FJsonValue>> PathArray;
    for (const FString& Path : ImportedAssetPaths)
    {
        PathArray.Add(MakeShared<FJsonValueString>(Path));
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("source_file"), SourceFile);
    Response->SetArrayField(TEXT("imported_assets"), PathArray);
    Response->SetStringField(TEXT("message"), TEXT("Asset imported successfully"));
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleExportAsset(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
    }

    FString DestinationFile;
    if (!Params->TryGetStringField(TEXT("destination_file"), DestinationFile))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'destination_file' parameter"));
    }

    if (!UEditorAssetLibrary::DoesAssetExist(AssetPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Asset not found: %s"), *AssetPath));
    }

    UObject* Asset = UEditorAssetLibrary::LoadAsset(AssetPath);
    if (!Asset)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load asset: %s"), *AssetPath));
    }

    UExporter::ExportToFile(Asset, nullptr, *DestinationFile, false);

    if (!FPaths::FileExists(DestinationFile))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to export asset"));
    }

    TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetStringField(TEXT("status"), TEXT("success"));
    Response->SetStringField(TEXT("asset_path"), AssetPath);
    Response->SetStringField(TEXT("destination_file"), DestinationFile);
    Response->SetStringField(TEXT("message"), TEXT("Asset exported successfully"));
    return Response;
}