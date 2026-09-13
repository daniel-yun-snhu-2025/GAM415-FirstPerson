// GAM 415 - Module 2: Dynamic Material Instance cube

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CubeDMIMod.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 *  Cube that creates a Dynamic Material Instance from a base material and
 *  randomizes its Color / Darkness / Opacity parameters when the player overlaps it.
 */
UCLASS()
class FIRSTPERSON415_API ACubeDMIMod : public AActor
{
	GENERATED_BODY()

public:
	ACubeDMIMod();

protected:
	virtual void BeginPlay() override;

public:
	/** Overlap volume, used as the root component */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	UBoxComponent* boxComp;

	/** Visible cube mesh that receives the dynamic material */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* cubeMesh;

	/** Base material the dynamic instance is created from (DMI1_Mat) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
	UMaterialInterface* baseMat;

	/** Dynamic material instance applied to the cube at BeginPlay */
	UPROPERTY()
	UMaterialInstanceDynamic* dmiMat;

	/** Called when something overlaps the box component */
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
