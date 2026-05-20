// Manages local variables within Blueprint function graphs

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Handles local variable creation, listing, and removal
 * within Blueprint function graphs (not class-level variables).
 */
class UNREALMCP_API FLocalVariableManager
{
public:
	/**
	 * Add a local variable to a Blueprint function graph.
	 * @param Params JSON containing:
	 *   - blueprint_name (string): Name of the Blueprint
	 *   - function_name (string): Name of the function graph
	 *   - variable_name (string): Name for the local variable
	 *   - variable_type (string): Type: bool, int, float, string, vector, rotator, object
	 *   - default_value (string, optional): Default value string
	 * @return JSON with local_variable info or error
	 */
	static TSharedPtr<FJsonObject> AddLocalVariable(const TSharedPtr<FJsonObject>& Params);

	/**
	 * List all local variables in a Blueprint function graph.
	 * @param Params JSON containing:
	 *   - blueprint_name (string): Name of the Blueprint
	 *   - function_name (string): Name of the function graph
	 * @return JSON with "local_variables" array or error
	 */
	static TSharedPtr<FJsonObject> GetLocalVariables(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Remove a local variable from a Blueprint function graph.
	 * @param Params JSON containing:
	 *   - blueprint_name (string): Name of the Blueprint
	 *   - function_name (string): Name of the function graph
	 *   - variable_name (string): Name of the variable to remove
	 * @return JSON with success or error
	 */
	static TSharedPtr<FJsonObject> RemoveLocalVariable(const TSharedPtr<FJsonObject>& Params);
};
