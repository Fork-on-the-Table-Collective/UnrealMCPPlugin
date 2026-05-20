// Manages Custom Event node creation and parameter addition

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Handles creation of Custom Event nodes and adding input parameters to them.
 */
class UNREALMCP_API FCustomEventManager
{
public:
	/**
	 * Add a Custom Event node to the Blueprint's event graph.
	 * @param Params JSON containing:
	 *   - blueprint_name (string): Name of the Blueprint
	 *   - event_name (string): Name for the custom event
	 *   - pos_x, pos_y (number, optional): Graph position
	 * @return JSON with node_id, event_name or error
	 */
	static TSharedPtr<FJsonObject> AddCustomEvent(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add an input parameter to an existing Custom Event node.
	 * @param Params JSON containing:
	 *   - blueprint_name (string): Name of the Blueprint
	 *   - event_name (string): Name of the existing custom event
	 *   - parameter_name (string): Name for the new parameter
	 *   - parameter_type (string): Type: bool, int, float, string, vector, rotator, object
	 * @return JSON with updated pin list or error
	 */
	static TSharedPtr<FJsonObject> AddEventParameter(const TSharedPtr<FJsonObject>& Params);
};
