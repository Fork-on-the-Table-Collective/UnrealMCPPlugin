// Header for creating loop nodes (ForLoop, WhileLoop, ForEachLoop)

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"

class UK2Node;

/**
 * Creator for Unreal Blueprint loop nodes
 * Handles ForLoop, WhileLoop, and ForEachLoop creation
 */
class FLoopNodeCreator
{
public:
	/**
	 * Creates a ForLoop node (K2Node_ForLoop or custom macro via WhileLoop)
	 * Generates: First, Last, Body, Completed, Index outputs
	 * @param Graph - The graph to add the node to
	 * @param Params - JSON parameters containing:
	 *                 - pos_x, pos_y: position
	 *                 - start_index (int, default 0): loop start
	 *                 - end_index (int, default 10): loop end (inclusive)
	 * @return The created node or nullptr on error
	 */
	static UK2Node* CreateForLoopNode(UEdGraph* Graph, const TSharedPtr<class FJsonObject>& Params);

	/**
	 * Creates a WhileLoop node via K2Node_WhileLoop
	 * Generates: Condition (bool input), Loop Body, Completed outputs
	 * @param Graph - The graph to add the node to
	 * @param Params - JSON parameters containing pos_x, pos_y
	 * @return The created node or nullptr on error
	 */
	static UK2Node* CreateWhileLoopNode(UEdGraph* Graph, const TSharedPtr<class FJsonObject>& Params);

	/**
	 * Creates a ForEachLoop node (K2Node_ForEachElement or Array ForEach macro)
	 * Generates: Array input, Loop Body, Element output, Completed
	 * @param Graph - The graph to add the node to
	 * @param Params - JSON parameters containing:
	 *                 - pos_x, pos_y: position
	 *                 - array_type (string): element type ("Object", "int", "float", etc.)
	 * @return The created node or nullptr on error
	 */
	static UK2Node* CreateForEachLoopNode(UEdGraph* Graph, const TSharedPtr<class FJsonObject>& Params);
};
