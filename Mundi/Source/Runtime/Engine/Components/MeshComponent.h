#pragma once
#include "PrimitiveComponent.h"

class UShader;

class UMeshComponent : public UPrimitiveComponent
{
public:
    DECLARE_CLASS(UMeshComponent, UPrimitiveComponent)
    GENERATED_REFLECTION_BODY()

    UMeshComponent();

protected:
    ~UMeshComponent() override;

public:
    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
    
    void DuplicateSubObjects() override;
    DECLARE_DUPLICATE(UMeshComponent)

// Material Section
public:
    UMaterialInterface* GetMaterial(uint32 InSectionIndex) const override;
    void SetMaterial(uint32 InElementIndex, UMaterialInterface* InNewMaterial) override;

    UMaterialInstanceDynamic* CreateAndSetMaterialInstanceDynamic(uint32 ElementIndex);

    const TArray<UMaterialInterface*>& GetMaterialSlots() const { return MaterialSlots; }

    void SetMaterialTextureByUser(const uint32 InMaterialSlotIndex, EMaterialTextureSlot Slot, UTexture* Texture);
    void SetMaterialColorByUser(const uint32 InMaterialSlotIndex, const FString& ParameterName, const FLinearColor& Value);
    void SetMaterialScalarByUser(const uint32 InMaterialSlotIndex, const FString& ParameterName, float Value);
    
protected:
    void ClearDynamicMaterials();
    
    TArray<UMaterialInterface*> MaterialSlots;
    TArray<UMaterialInstanceDynamic*> DynamicMaterialInstances;

// Shadow Section
public:
    bool IsCastShadows() const { return bCastShadows; }

private:
    bool bCastShadows = true;
};