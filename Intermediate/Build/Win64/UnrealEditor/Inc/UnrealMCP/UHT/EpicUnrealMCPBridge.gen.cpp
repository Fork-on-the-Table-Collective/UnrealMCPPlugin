// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "EpicUnrealMCPBridge.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeEpicUnrealMCPBridge() {}

// ********** Begin Cross Module References ********************************************************
EDITORSUBSYSTEM_API UClass* Z_Construct_UClass_UEditorSubsystem();
UNREALMCP_API UClass* Z_Construct_UClass_UEpicUnrealMCPBridge();
UNREALMCP_API UClass* Z_Construct_UClass_UEpicUnrealMCPBridge_NoRegister();
UPackage* Z_Construct_UPackage__Script_UnrealMCP();
// ********** End Cross Module References **********************************************************

// ********** Begin Class UEpicUnrealMCPBridge *****************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_UEpicUnrealMCPBridge;
UClass* UEpicUnrealMCPBridge::GetPrivateStaticClass()
{
	using TClass = UEpicUnrealMCPBridge;
	if (!Z_Registration_Info_UClass_UEpicUnrealMCPBridge.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("EpicUnrealMCPBridge"),
			Z_Registration_Info_UClass_UEpicUnrealMCPBridge.InnerSingleton,
			StaticRegisterNativesUEpicUnrealMCPBridge,
			sizeof(TClass),
			alignof(TClass),
			TClass::StaticClassFlags,
			TClass::StaticClassCastFlags(),
			TClass::StaticConfigName(),
			(UClass::ClassConstructorType)InternalConstructor<TClass>,
			(UClass::ClassVTableHelperCtorCallerType)InternalVTableHelperCtorCaller<TClass>,
			UOBJECT_CPPCLASS_STATICFUNCTIONS_FORCLASS(TClass),
			&TClass::Super::StaticClass,
			&TClass::WithinClass::StaticClass
		);
	}
	return Z_Registration_Info_UClass_UEpicUnrealMCPBridge.InnerSingleton;
}
UClass* Z_Construct_UClass_UEpicUnrealMCPBridge_NoRegister()
{
	return UEpicUnrealMCPBridge::GetPrivateStaticClass();
}
struct Z_Construct_UClass_UEpicUnrealMCPBridge_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Editor subsystem for MCP Bridge\n * Handles communication between external tools and the Unreal Editor\n * through a TCP socket connection. Commands are received as JSON and\n * routed to appropriate command handlers.\n */" },
#endif
		{ "IncludePath", "EpicUnrealMCPBridge.h" },
		{ "ModuleRelativePath", "Public/EpicUnrealMCPBridge.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Editor subsystem for MCP Bridge\nHandles communication between external tools and the Unreal Editor\nthrough a TCP socket connection. Commands are received as JSON and\nrouted to appropriate command handlers." },
#endif
	};
#endif // WITH_METADATA

// ********** Begin Class UEpicUnrealMCPBridge constinit property declarations *********************
// ********** End Class UEpicUnrealMCPBridge constinit property declarations ***********************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UEpicUnrealMCPBridge>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_UEpicUnrealMCPBridge_Statics
UObject* (*const Z_Construct_UClass_UEpicUnrealMCPBridge_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UEditorSubsystem,
	(UObject* (*)())Z_Construct_UPackage__Script_UnrealMCP,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UEpicUnrealMCPBridge_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UEpicUnrealMCPBridge_Statics::ClassParams = {
	&UEpicUnrealMCPBridge::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	0,
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UEpicUnrealMCPBridge_Statics::Class_MetaDataParams), Z_Construct_UClass_UEpicUnrealMCPBridge_Statics::Class_MetaDataParams)
};
void UEpicUnrealMCPBridge::StaticRegisterNativesUEpicUnrealMCPBridge()
{
}
UClass* Z_Construct_UClass_UEpicUnrealMCPBridge()
{
	if (!Z_Registration_Info_UClass_UEpicUnrealMCPBridge.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UEpicUnrealMCPBridge.OuterSingleton, Z_Construct_UClass_UEpicUnrealMCPBridge_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UEpicUnrealMCPBridge.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, UEpicUnrealMCPBridge);
// ********** End Class UEpicUnrealMCPBridge *******************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_Users_guber_work_repos_ViridianCo_Plugins_UnrealMCP_Source_UnrealMCP_Public_EpicUnrealMCPBridge_h__Script_UnrealMCP_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UEpicUnrealMCPBridge, UEpicUnrealMCPBridge::StaticClass, TEXT("UEpicUnrealMCPBridge"), &Z_Registration_Info_UClass_UEpicUnrealMCPBridge, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UEpicUnrealMCPBridge), 2936459795U) },
	};
}; // Z_CompiledInDeferFile_FID_Users_guber_work_repos_ViridianCo_Plugins_UnrealMCP_Source_UnrealMCP_Public_EpicUnrealMCPBridge_h__Script_UnrealMCP_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_guber_work_repos_ViridianCo_Plugins_UnrealMCP_Source_UnrealMCP_Public_EpicUnrealMCPBridge_h__Script_UnrealMCP_3084867270{
	TEXT("/Script/UnrealMCP"),
	Z_CompiledInDeferFile_FID_Users_guber_work_repos_ViridianCo_Plugins_UnrealMCP_Source_UnrealMCP_Public_EpicUnrealMCPBridge_h__Script_UnrealMCP_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_guber_work_repos_ViridianCo_Plugins_UnrealMCP_Source_UnrealMCP_Public_EpicUnrealMCPBridge_h__Script_UnrealMCP_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
