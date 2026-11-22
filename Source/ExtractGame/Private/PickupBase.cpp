#include "PickupBase.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Font.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Math/RotationMatrix.h"
#include "Engine/Font.h"                    // UFont（离线字体）
#include "UObject/ConstructorHelpers.h"     // FObjectFinder（构造函数里按路径找资产）

APickupBase::APickupBase()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	// 检测球：只和 Pawn 重叠，用来显示"按 F 拾取"
	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphere->SetupAttachment(MeshComponent);
	DetectionSphere->SetSphereRadius(300.f);
	DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionSphere->SetGenerateOverlapEvents(true);
	DetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &APickupBase::OnDetectionBegin);
	DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &APickupBase::OnDetectionEnd);

	// 头顶文字
	Name = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Name"));
	Name->SetupAttachment(MeshComponent);
	Name->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	Name->SetHorizontalAlignment(EHTA_Center);
	Name->SetWorldSize(30.f);
	Name->SetVisibility(false);

	// 所有拾取物默认用项目的离线中文字体 + 对应的文本材质
	// （离线字体必须配带 "Font" 字体参数的材质，引擎会自动把字符图集绑进去）
	static ConstructorHelpers::FObjectFinder<UFont> OfflineFont(TEXT("/Game/UI/Fonts/bb4171_Font.bb4171_Font"));
	if (OfflineFont.Succeeded())
	{
		Name->SetFont(OfflineFont.Object);            // SetFont 会同时算好字符高度缩放
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LootTextMat(TEXT("/Game/UI/Fonts/M_LootTextAlpha.M_LootTextAlpha"));
	if (LootTextMat.Succeeded())
	{
		Name->SetTextMaterial(LootTextMat.Object);    // 材质带 Font 参数，引擎负责绑定图集
	}
}

void APickupBase::BeginPlay()
{
	Super::BeginPlay();

	// 入场开物理，落地后自动关
	if (!bDropped && bSettleOnBeginPlay && MeshComponent && MeshComponent->GetStaticMesh())
	{
		if (DroppedPhysics())
		{
			bSettling = true;
			MeshComponent->OnComponentHit.AddDynamic(this, &APickupBase::OnPickupHit);
		}
	}

	// 头顶文字的材质处理：
	//   默认（bUseDefaultTextMaterial = true）什么都不要做！
	//   TextRender 会用它自带的默认文本材质，引擎内部（FTextRenderComponentMIDCache）
	//   会自动为离线字体创建 MID 并绑定字符图集。我们再套一层 MID 会把它的缓存顶掉，
	//   结果就是"整块方块"。
	if (Name && bUseDefaultTextMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Pickup文字] %s | 引擎默认材质(不覆盖) | 字体=%s | 离线=%d | 当前文本材质=%s"),
			*GetName(),
			*GetNameSafe(Name->Font),
			(Name->Font && Name->Font->FontCacheType == EFontCacheType::Offline) ? 1 : 0,
			*GetNameSafe(Name->TextMaterial));
	}
	else if (Name && Name->Font && Name->Font->FontCacheType == EFontCacheType::Offline)
	{
		// 只有明确要自定义材质时，才手动绑字体图集
		UMaterialInterface* BaseMaterial = Name->TextMaterial;
		if (!BaseMaterial && Name->GetNumMaterials() > 0) { BaseMaterial = Name->GetMaterial(0); }

		if (BaseMaterial)
		{
			if (UMaterialInstanceDynamic* FontMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this))
			{
				FontMaterial->SetFontParameterValue(FMaterialParameterInfo(FName(TEXT("Font"))), Name->Font, 0);
				Name->SetTextMaterial(FontMaterial);

				UE_LOG(LogTemp, Warning, TEXT("[Pickup文字] %s | 自定义材质=%s | 字体=%s"),
					*GetName(), *GetNameSafe(BaseMaterial), *GetNameSafe(Name->Font));
			}
		}
	}
}

void APickupBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 落地流程：先等 0.3 秒，再看物理是否休眠；入场落地另有 5 秒兜底
	if (DrpToStl && MeshComponent)
	{
		DrpPhysicsElapsed += DeltaTime;

		if (bSettling && DrpPhysicsElapsed > 5.f)
		{
			StopSettlePhysics();
		}
		else if (DrpPhysicsElapsed >= 0.3f && !MeshComponent->IsAnyRigidBodyAwake())
		{
			if (bSettling)
			{
				StopSettlePhysics();
			}
			else
			{
				MeshComponent->SetSimulatePhysics(false);
				DrpToStl = false;
			}
		}
	}

	// 头顶文字：跟随网格顶部，并始终面向摄像机
	if (Name == nullptr || !Name->IsVisible() || MeshComponent == nullptr) { return; }

	Name->SetWorldLocation(
		MeshComponent->Bounds.Origin
		+ FVector(0.f, 0.f, MeshComponent->Bounds.BoxExtent.Z + 30.f));

	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (PlayerController == nullptr || PlayerController->PlayerCameraManager == nullptr) { return; }

	const FVector CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
	FVector DirectionCamera = CameraLocation - Name->GetComponentLocation();

	if (!DirectionCamera.IsNearlyZero())
	{
		const FRotator FacingRotation = FRotationMatrix::MakeFromXZ(DirectionCamera.GetSafeNormal(), FVector::UpVector).Rotator();
		Name->SetWorldRotation(FacingRotation);
	}
}

void APickupBase::InitDroppedPickup(FName InId, int32 InValue, int32 InCount)
{
	bDropped = true;                        // 丢弃物不走进场落地，直接由调用方开物理
	ApplyPickupData(InId, InValue);         // 子类把 ID/价值写进自己的字段
	ApplyPickupCount(InCount);              // 子类把数量写进自己的字段（子弹盒这类才用得上）
}

void APickupBase::OnDetectionBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!Cast<ACharacter>(OtherActor) || Name == nullptr) { return; }

	const FString PromptText = GetPickupPrompt();
	if (PromptText.IsEmpty()) { return; }    // 子类没提供提示就不显示

	Name->SetText(FText::FromString(PromptText));
	Name->SetVisibility(true);
	Name->SetWorldSize(30.f);
	Name->SetTextRenderColor(FColor::Black);
}

void APickupBase::OnDetectionEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (Cast<ACharacter>(OtherActor) && Name != nullptr)
	{
		Name->SetVisibility(false);
	}
}

bool APickupBase::DroppedPhysics()
{
	if (!MeshComponent || !MeshComponent->GetStaticMesh()) { return false; }

	MeshComponent->SetMobility(EComponentMobility::Movable);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionObjectType(ECC_PhysicsBody);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComponent->SetSimulatePhysics(true);
	MeshComponent->SetEnableGravity(true);

	DrpPhysicsElapsed = 0.f;
	DrpToStl = MeshComponent->IsSimulatingPhysics();
	return DrpToStl;
}

void APickupBase::OnPickupHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!bSettling) { return; }               // 只有入场落地阶段需要处理
	if (Hit.ImpactNormal.Z < 0.5f) { return; } // 撞墙/斜坡不算落地
	StopSettlePhysics();
}

void APickupBase::StopSettlePhysics()
{
	if (!MeshComponent) { return; }

	bSettling = false;
	DrpToStl = false;
	MeshComponent->SetSimulatePhysics(false);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComponent->OnComponentHit.RemoveDynamic(this, &APickupBase::OnPickupHit);
}
