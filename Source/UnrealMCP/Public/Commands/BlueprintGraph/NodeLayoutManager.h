// Handles node layout/arrangement in Blueprint graphs

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Arranges or sets the positions of nodes in a Blueprint graph.
 */
class UNREALMCP_API FNodeLayoutManager
{
public:
	/**
	 * Automatically arrange all nodes in a grid layout.
	 * @param Params JSON containing:
	 *   - blueprint_name (string): Name of the Blueprint
	 *   - function_name (string, optional): Target function graph (uses event graph if omitted)
	 *   - spacing_x (number, optional): Horizontal spacing between nodes (default 300)
	 *   - spacing_y (number, optional): Vertical spacing between nodes (default 200)
	 * @return JSON with node_count or error
	 */
	static TSharedPtr<FJsonObject> ArrangeNodesAuto(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Arrange all nodes in a horizontal line following execution order.
	 * @param Params JSON containing:
	 *   - blueprint_name (string): Name of the Blueprint
	 *   - function_name (string, optional): Target function graph (uses event graph if omitted)
	 *   - start_x (number, optional): Starting X position (default 0)
	 *   - start_y (number, optional): Starting Y position (default 0)
	 *   - spacing_x (number, optional): Horizontal spacing (default 300)
	 * @return JSON with node_count or error
	 */
	static TSharedPtr<FJsonObject> ArrangeNodesLinear(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Set the position of a specific node by its GUID or title.
	 * @param Params JSON containing:
	 *   - blueprint_name (string): Name of the Blueprint
	 *   - node_id (string): GUID of the node to move
	 *   - pos_x (number): Target X position
	 *   - pos_y (number): Target Y position
	 *   - function_name (string, optional): Graph to search in
	 * @return JSON with success or error
	 */
	static TSharedPtr<FJsonObject> SetNodePosition(const TSharedPtr<FJsonObject>& Params);
};
