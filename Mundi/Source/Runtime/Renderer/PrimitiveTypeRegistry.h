#pragma once

#include <limits>

#include "UEContainer.h"

struct UClass;

/**
 * @brief 프리미티브 컴포넌트 타입을 uint16 ID에 매핑하는 레지스트리
 */
class FPrimitiveTypeRegistry
{
public:
	static constexpr uint16 InvalidTypeId = std::numeric_limits<uint16>::max();

	static FPrimitiveTypeRegistry& GetInstance();

	uint16 RegisterType(const UClass* InClass);
	uint16 GetTypeId(const UClass* InClass) const;
	const UClass* ResolveType(uint16 TypeId) const;
	void Reset();

private:
	FPrimitiveTypeRegistry() = default;

	bool IsPrimitiveClass(const UClass* InClass) const;
	uint16 AllocateTypeId(const UClass* InClass);

	TArray<const UClass*> RegisteredTypes; // TypeId -> UClass
	TMap<const UClass*, uint16> TypeLookup; // UClass -> TypeId
};
