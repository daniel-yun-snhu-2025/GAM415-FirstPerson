// GAM 415 - Module 2: Dynamic Material Instance cube

#include "CubeDMIMod.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/KismetMathLibrary.h"
#include "firstperson415Character.h"

ACubeDMIMod::ACubeDMIMod()
{
	PrimaryActorTick.bCanEverTick = false;

	// create the sub objects
	boxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("Box Component"));
	cubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cube Mesh"));

	// set up attachments: box is the root, the cube mesh hangs off it
	RootComponent = boxComp;
	cubeMesh->SetupAttachment(boxComp);

	boxComp->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));
	boxComp->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	boxComp->SetGenerateOverlapEvents(true);
}

void ACubeDMIMod::BeginPlay()
{
	Super::BeginPlay();

	// listen for overlaps on the box
	boxComp->OnComponentBeginOverlap.AddDynamic(this, &ACubeDMIMod::OnOverlapBegin);

	// create the dynamic material instance from the base material and apply it to the cube
	if (baseMat)
	{
		dmiMat = UMaterialInstanceDynamic::Create(baseMat, this);

		if (cubeMesh)
		{
			cubeMesh->SetMaterial(0, dmiMat);
		}
	}
}

void ACubeDMIMod::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// only react to the player character
	Afirstperson415Character* overlappedActor = Cast<Afirstperson415Character>(OtherActor);

	if (overlappedActor && dmiMat)
	{
		// random RGB in [0,1]
		float ranNumX = UKismetMathLibrary::RandomFloatInRange(0.0f, 1.0f);
		float ranNumY = UKismetMathLibrary::RandomFloatInRange(0.0f, 1.0f);
		float ranNumZ = UKismetMathLibrary::RandomFloatInRange(0.0f, 1.0f);

		// alpha stays 1
		FLinearColor randColor = FLinearColor(ranNumX, ranNumY, ranNumZ, 1.0f);

		dmiMat->SetVectorParameterValue(TEXT("Color"), randColor);
		dmiMat->SetScalarParameterValue(TEXT("Darkness"), ranNumX);

		// assignment requirement: also randomize the opacity (kept in a visible range)
		float ranOpacity = UKismetMathLibrary::RandomFloatInRange(0.2f, 1.0f);
		dmiMat->SetScalarParameterValue(TEXT("Opacity"), ranOpacity);
	}
}
