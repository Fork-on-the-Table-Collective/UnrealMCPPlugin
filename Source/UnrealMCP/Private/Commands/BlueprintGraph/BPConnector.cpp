#include "Commands/BlueprintGraph/BPConnector.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "Engine/Blueprint.h"
#include "K2Node.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EditorAssetLibrary.h"

TSharedPtr<FJsonObject> FBPConnector::ConnectNodes(const TSharedPtr<FJsonObject>& Params)
{
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();

    // Extraire paramètres
    FString BlueprintName = Params->GetStringField(TEXT("blueprint_name"));
    FString SourceNodeId = Params->GetStringField(TEXT("source_node_id"));
    FString SourcePinName = Params->GetStringField(TEXT("source_pin_name"));
    FString TargetNodeId = Params->GetStringField(TEXT("target_node_id"));
    FString TargetPinName = Params->GetStringField(TEXT("target_pin_name"));

    FString FunctionName;
    Params->TryGetStringField(TEXT("function_name"), FunctionName);

    // Charger Blueprint - handle both full paths and simple names
    UBlueprint* Blueprint = nullptr;
    FString BlueprintPath = BlueprintName;

    // If no path prefix, assume /Game/Blueprints/
    if (!BlueprintPath.StartsWith(TEXT("/")))
    {
        BlueprintPath = TEXT("/Game/Blueprints/") + BlueprintPath;
    }

    // Add .Blueprint suffix if not present
    if (!BlueprintPath.Contains(TEXT(".")))
    {
        BlueprintPath += TEXT(".") + FPaths::GetBaseFilename(BlueprintPath);
    }

    // Try to load the Blueprint
    Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);

    // If not found, try with UEditorAssetLibrary
    if (!Blueprint)
    {
        FString AssetPath = BlueprintPath;
        if (UEditorAssetLibrary::DoesAssetExist(AssetPath))
        {
            UObject* Asset = UEditorAssetLibrary::LoadAsset(AssetPath);
            Blueprint = Cast<UBlueprint>(Asset);
        }
    }

    if (!Blueprint)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Blueprint not found");
        return Result;
    }

    // Get graph
    UEdGraph* Graph = nullptr;

    if (!FunctionName.IsEmpty())
    {
        // Strategy 1: Try exact name match with GetFName()
        for (UEdGraph* FuncGraph : Blueprint->FunctionGraphs)
        {
            if (FuncGraph && (FuncGraph->GetFName().ToString() == FunctionName ||
                              (FuncGraph->GetOuter() && FuncGraph->GetOuter()->GetFName().ToString() == FunctionName)))
            {
                Graph = FuncGraph;
                break;
            }
        }

        // Strategy 2: Fallback - partial match for auto-generated names
        if (!Graph)
        {
            for (UEdGraph* FuncGraph : Blueprint->FunctionGraphs)
            {
                if (FuncGraph && FuncGraph->GetFName().ToString().Contains(FunctionName))
                {
                    Graph = FuncGraph;
                    break;
                }
            }
        }

        if (!Graph)
        {
            Result->SetBoolField("success", false);
            Result->SetStringField("error", FString::Printf(TEXT("Function graph not found: %s"), *FunctionName));
            return Result;
        }
    }
    else
    {
        // Use event graph if no function specified
        if (Blueprint->UbergraphPages.Num() == 0)
        {
            Result->SetBoolField("success", false);
            Result->SetStringField("error", "Blueprint has no event graph");
            return Result;
        }

        Graph = Blueprint->UbergraphPages[0];
    }

    if (!Graph)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Graph not found");
        return Result;
    }

    // Find nodes
    UK2Node* SourceNode = FindNodeById(Graph, SourceNodeId);
    UK2Node* TargetNode = FindNodeById(Graph, TargetNodeId);

    if (!SourceNode || !TargetNode)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Node not found");
        return Result;
    }

    // Trouver pins
    UEdGraphPin* SourcePin = FindPinByName(SourceNode, SourcePinName, EGPD_Output);
    UEdGraphPin* TargetPin = FindPinByName(TargetNode, TargetPinName, EGPD_Input);

    if (!SourcePin || !TargetPin)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Pin not found");
        return Result;
    }

    // Let the graph schema validate and make the link, the same way the editor does. Unlike a plain
    // pin-category comparison, the schema resolves wildcard pins (e.g. a Cast node's Object input,
    // reroute nodes, array functions), rejects mismatched object/struct types, and notifies the nodes
    // so wildcard pins pick up their type.
    // Note: like the editor, linking into a data input that already has a link replaces that link,
    // and linking from an exec output that already has a link moves it.
    const UEdGraphSchema* Schema = Graph->GetSchema();
    if (!Schema)
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Graph has no schema");
        return Result;
    }

    const FPinConnectionResponse Response = Schema->CanCreateConnection(SourcePin, TargetPin);
    if (Response.Response == CONNECT_RESPONSE_DISALLOW)
    {
        const FString Reason = Response.Message.IsEmpty() ? TEXT("no reason given") : Response.Message.ToString();
        Result->SetBoolField("success", false);
        Result->SetStringField("error", FString::Printf(TEXT("Pins not compatible: %s"), *Reason));
        return Result;
    }

    // Snapshot every link on both nodes so we can report exactly which ones the schema removed.
    // Links can be broken by more than BREAK_OTHERS responses: an automatic conversion node replaces
    // the target input's existing link, and wildcard type resolution can drop now-incompatible links.
    const TMap<FString, TSharedPtr<FJsonObject>> LinksBefore = CollectNodeLinks({ SourceNode, TargetNode });

    if (!Schema->TryCreateConnection(SourcePin, TargetPin))
    {
        Result->SetBoolField("success", false);
        Result->SetStringField("error", "Schema refused the connection");
        return Result;
    }

    const TMap<FString, TSharedPtr<FJsonObject>> LinksAfter = CollectNodeLinks({ SourceNode, TargetNode });
    TArray<TSharedPtr<FJsonValue>> RemovedLinks;
    for (const TPair<FString, TSharedPtr<FJsonObject>>& Link : LinksBefore)
    {
        if (!LinksAfter.Contains(Link.Key))
        {
            RemovedLinks.Add(MakeShared<FJsonValueObject>(Link.Value));
        }
    }

    // Recompile
    Blueprint->MarkPackageDirty();
    FKismetEditorUtilities::CompileBlueprint(Blueprint);

    // Return
    Result->SetBoolField("success", true);

    TSharedPtr<FJsonObject> ConnectionInfo = MakeShared<FJsonObject>();
    ConnectionInfo->SetStringField("source_node", SourceNodeId);
    ConnectionInfo->SetStringField("source_pin", SourcePinName);
    ConnectionInfo->SetStringField("target_node", TargetNodeId);
    ConnectionInfo->SetStringField("target_pin", TargetPinName);
    ConnectionInfo->SetStringField("connection_type", SourcePin->PinType.PinCategory.ToString());
    ConnectionInfo->SetBoolField("replaced_existing_links", RemovedLinks.Num() > 0);
    ConnectionInfo->SetArrayField("removed_links", RemovedLinks);
    ConnectionInfo->SetBoolField("conversion_node_created", Response.Response == CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE);
    if (!Response.Message.IsEmpty())
    {
        ConnectionInfo->SetStringField("schema_message", Response.Message.ToString());
    }

    Result->SetObjectField("connection", ConnectionInfo);

    return Result;
}

UK2Node* FBPConnector::FindNodeById(UEdGraph* Graph, const FString& NodeId)
{
    if (!Graph)
    {
        return nullptr;
    }

    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node)
        {
            continue;
        }

        // Try matching by NodeGuid first
        if (Node->NodeGuid.ToString().Equals(NodeId, ESearchCase::IgnoreCase))
        {
            UK2Node* K2Node = Cast<UK2Node>(Node);
            return K2Node;  // Return even if nullptr (caller will handle)
        }

        // Try matching by GetName()
        if (Node->GetName().Equals(NodeId, ESearchCase::IgnoreCase))
        {
            UK2Node* K2Node = Cast<UK2Node>(Node);
            return K2Node;  // Return even if nullptr (caller will handle)
        }
    }

    return nullptr;
}

UEdGraphPin* FBPConnector::FindPinByName(UK2Node* Node, const FString& PinName, EEdGraphPinDirection Direction)
{
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->PinName.ToString() == PinName && Pin->Direction == Direction)
        {
            return Pin;
        }
    }
    return nullptr;
}

TMap<FString, TSharedPtr<FJsonObject>> FBPConnector::CollectNodeLinks(const TArray<UEdGraphNode*>& Nodes)
{
    TMap<FString, TSharedPtr<FJsonObject>> Links;

    for (const UEdGraphNode* Node : Nodes)
    {
        if (!Node)
        {
            continue;
        }

        for (const UEdGraphPin* Pin : Node->Pins)
        {
            if (!Pin)
            {
                continue;
            }

            for (const UEdGraphPin* LinkedPin : Pin->LinkedTo)
            {
                if (!LinkedPin || !LinkedPin->GetOwningNodeUnchecked())
                {
                    continue;
                }

                // Orient every link output -> input so both ends produce the same key.
                const UEdGraphPin* OutputPin = Pin->Direction == EGPD_Output ? Pin : LinkedPin;
                const UEdGraphPin* InputPin = Pin->Direction == EGPD_Output ? LinkedPin : Pin;
                const UEdGraphNode* OutputNode = OutputPin->GetOwningNode();
                const UEdGraphNode* InputNode = InputPin->GetOwningNode();
                const FString OutputPinName = OutputPin->PinName.ToString();
                const FString InputPinName = InputPin->PinName.ToString();

                const FString Key = FString::Printf(TEXT("%s.%s->%s.%s"),
                    *OutputNode->NodeGuid.ToString(), *OutputPinName, *InputNode->NodeGuid.ToString(), *InputPinName);
                if (Links.Contains(Key))
                {
                    continue;
                }

                // Report node names: that is the node_id add_blueprint_node returns, and FindNodeById accepts it.
                TSharedPtr<FJsonObject> LinkInfo = MakeShared<FJsonObject>();
                LinkInfo->SetStringField("source_node", OutputNode->GetName());
                LinkInfo->SetStringField("source_pin", OutputPinName);
                LinkInfo->SetStringField("target_node", InputNode->GetName());
                LinkInfo->SetStringField("target_pin", InputPinName);
                Links.Add(Key, LinkInfo);
            }
        }
    }

    return Links;
}
