#include "pch.h"
#include "PrimitiveTypeRegistry.h"

#include "Object.h"
#include "PrimitiveComponent.h"

FPrimitiveTypeRegistry& FPrimitiveTypeRegistry::GetInstance()
{
	static FPrimitiveTypeRegistry Instance;
	return Instance;
}

uint16 FPrimitiveTypeRegistry::RegisterType(const UClass* InClass)
{
	if (!InClass)
	{
		return InvalidTypeId;
	}

	if (!IsPrimitiveClass(InClass))
	{
		return InvalidTypeId;
	}

	if (const uint16* ExistingId = TypeLookup.Find(InClass))
	{
		return *ExistingId;
	}

	return AllocateTypeId(InClass);
}

uint16 FPrimitiveTypeRegistry::GetTypeId(const UClass* InClass) const
{
	if (!InClass)
	{
		return InvalidTypeId;
	}

	if (!IsPrimitiveClass(InClass))
	{
		return InvalidTypeId;
	}

	if (const uint16* ExistingId = TypeLookup.Find(InClass))
	{
		return *ExistingId;
	}

	return InvalidTypeId;
}

const UClass* FPrimitiveTypeRegistry::ResolveType(uint16 TypeId) const
{
	if (TypeId == InvalidTypeId)
	{
		return nullptr;
	}

	if (TypeId >= static_cast<uint16>(RegisteredTypes.Num()))
	{
		return nullptr;
	}

	return RegisteredTypes[TypeId];
}

void FPrimitiveTypeRegistry::Reset()
{
	RegisteredTypes.clear();
	TypeLookup.clear();
}

bool FPrimitiveTypeRegistry::IsPrimitiveClass(const UClass* InClass) const
{
	if (!InClass)
	{
		return false;
	}

	const UClass* PrimitiveBase = UPrimitiveComponent::StaticClass();
	return (PrimitiveBase != nullptr) && InClass->IsChildOf(PrimitiveBase);
}

uint16 FPrimitiveTypeRegistry::AllocateTypeId(const UClass* InClass)
{
	if (!IsPrimitiveClass(InClass))
	{
		return InvalidTypeId;
	}

	if (RegisteredTypes.Num() >= std::numeric_limits<uint16>::max())
	{
		return InvalidTypeId;
	}

	const uint16 NewId = static_cast<uint16>(RegisteredTypes.Num());
	RegisteredTypes.Add(InClass);
	TypeLookup[InClass] = NewId;
	UE_LOG("[PrimitiveTypeRegistry] Primitive Type Registered: %s - %u", InClass->Name, NewId);
	return NewId;
}
