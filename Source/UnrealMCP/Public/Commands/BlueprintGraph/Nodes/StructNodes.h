// Handles creation of MakeStruct and BreakStruct nodes in Blueprint graphs

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"

class UK2Node;

/**
 * Creator for struct operation nodes (MakeStruct, BreakStruct)
 */
class FStructNodeCreator
{
public:
	/**
	 * Creates a MakeStruct node for the named struct type.
	 * @param Graph - The graph to add the node to
	 * @param Params - JSON parameters containing:
	 *                 - struct_type (string): Asset path or short name of the UStruct
	 *                 - pos_x, pos_y: position
	 * @return The created node or nullptr on error
	 */
	static UK2Node* CreateMakeStructNode(UEdGraph* Graph, const TSharedPtr<class FJsonObject>& Params);

	/**
	 * Creates a BreakStruct node for the named struct type.
	 * @param Graph - The graph to add the node to
	 * @param Params - JSON parameters containing:
	 *                 - struct_type (string): Asset path or short name of the UStruct
	 *                 - pos_x, pos_y: position
	 * @return The created node or nullptr on error
	 */
	static UK2Node* CreateBreakStructNode(UEdGraph* Graph, const TSharedPtr<class FJsonObject>& Params);

private:
	/** Resolves a struct type from a short name or full asset path. Returns nullptr if not found. */
	static UScriptStruct* FindStructByName(const FString& StructName);
};
