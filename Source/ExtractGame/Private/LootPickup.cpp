// Fill out your copyright notice in the Description page of Project Settings.


#include "LootPickup.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Font.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Math/RotationMatrix.h"	//计算文字朝向

// Sets default values
ALootPickup::ALootPickup()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	//创建检测范围球体组件
	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphere->SetupAttachment(MeshComponent);
	DetectionSphere->SetSphereRadius(300.f);//设置检测范围半径为300
	DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionSphere->SetGenerateOverlapEvents(true);//启用重叠事件
	DetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);//只对Pawn通道（角色）产生重叠事件
	DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ALootPickup::OnDetectionBegin);
	DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &ALootPickup::OnDetectionEnd);
	//创建文本渲染组件
	Name = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Name"));
	Name->SetupAttachment(MeshComponent);
	Name->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	Name->SetHorizontalAlignment(EHTA_Center);
	Name->SetWorldSize(30.f);
	Name->SetVisibility(false);//默认不显示战利品名称
}

// Called when the game starts or when spawned
void ALootPickup::BeginPlay()
{
	Super::BeginPlay();
	// 显式把离线字体的第 0 张图集绑定到 TextRender 材质，避免运行时只绘制字符矩形。
	if (Name && Name->Font && Name->TextMaterial && Name->Font->FontCacheType == EFontCacheType::Offline)
	{
		if (UMaterialInstanceDynamic* FontMaterial = UMaterialInstanceDynamic::Create(Name->TextMaterial, this))
		{
			FontMaterial->SetFontParameterValue(FMaterialParameterInfo(FName(TEXT("Font"))), Name->Font, 0);
			Name->SetTextMaterial(FontMaterial);
		}
	}
}

// Called every frame
void ALootPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Name == nullptr || !Name->IsVisible()) { return; }//如果文本渲染组件为空或者不可见，则不执行后续代码

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController == nullptr || PlayerController->PlayerCameraManager == nullptr) { return; }//如果玩家控制器为空或者玩家摄像机管理器为空，则不执行后续代码

	const FVector CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();//获取摄像机位置
	FVector DirectionCamera = CameraLocation - Name->GetComponentLocation();//计算摄像机位置与文本渲染组件位置的方向向量

	if (!DirectionCamera.IsNearlyZero()){
		const FRotator FacingRotation = FRotationMatrix::MakeFromXZ(DirectionCamera.GetSafeNormal(), FVector::UpVector).Rotator();//计算文本渲染组件的朝向旋转，使其面向摄像机
		Name->SetWorldRotation(FacingRotation);//设置文本渲染组件的世界旋转，使其面向摄像机
	}

}

void ALootPickup::OnDetectionBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Cast<ACharacter>(OtherActor)) {
		//设置战利品名称
		const FString PromptText = FString::Printf(TEXT("%s(#%d)\n按[F]拾取"), *LootName.ToString(), LootValue);
		/*UE_LOG(LogTemp, Warning, TEXT("拾取提示诊断：文本=[%s]，字体=[%s]，缓存=[%s]，字符数=%d，纹理页数=%d，Remapped=%d"),
			*PromptText,
			*GetNameSafe(Name->Font),
			(Name->Font && Name->Font->FontCacheType == EFontCacheType::Offline) ? TEXT("Offline") : TEXT("Runtime/None"),
			Name->Font ? Name->Font->Characters.Num() : 0,
			Name->Font ? Name->Font->Textures.Num() : 0,
			Name->Font ? Name->Font->IsRemapped : 0);
		UE_LOG(LogTemp, Warning, TEXT("拾取提示诊断：文字材质=[%s]，字库纹理=[%s]"),
			*GetNameSafe(Name->TextMaterial),
			(Name->Font && Name->Font->Textures.IsValidIndex(0)) ? *GetNameSafe(Name->Font->Textures[0]) : TEXT("无有效纹理"));
		if (Name->TextMaterial)
		{
			TArray<FMaterialParameterInfo> FontParameters;
			TArray<FGuid> FontParameterIds;
			Name->TextMaterial->GetAllFontParameterInfo(FontParameters, FontParameterIds);
			UE_LOG(LogTemp, Warning, TEXT("字体材质参数诊断：字体参数数量=%d"), FontParameters.Num());
			for (const FMaterialParameterInfo& Parameter : FontParameters)
			{
				UE_LOG(LogTemp, Warning, TEXT("字体材质参数诊断：参数名=[%s]"), *Parameter.Name.ToString());
			}
		}
		if (Name->Font)
		{
			for (TCHAR OriginalChar : PromptText)
			{
				if (OriginalChar <= 255)
				{
					continue;
				}

				const TCHAR MappedChar = Name->Font->RemapChar(OriginalChar);
				if (!Name->Font->Characters.IsValidIndex(MappedChar))
				{
					UE_LOG(LogTemp, Warning, TEXT("字体字形诊断：U+%04X 没有有效映射，映射结果=%d"), (uint32)OriginalChar, (int32)MappedChar);
					continue;
				}

				const FFontCharacter& Glyph = Name->Font->Characters[MappedChar];
				UE_LOG(LogTemp, Warning, TEXT("字体字形诊断：U+%04X -> 索引%d，图集区域=(%d,%d,%d,%d)，纹理页=%d"),
					(uint32)OriginalChar, (int32)MappedChar, Glyph.StartU, Glyph.StartV, Glyph.USize, Glyph.VSize, (int32)Glyph.TextureIndex);
			}
		}*/
		Name->SetText(FText::FromString(PromptText));
		Name->SetVisibility(true);//显示战利品名称

		Name->SetRelativeLocation(FVector(0.f, 0.f, 200.f));
		Name->SetWorldSize(30.f);
		Name->SetTextRenderColor(FColor::Black);
	}
}

void ALootPickup::OnDetectionEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (Cast<ACharacter>(OtherActor)) {
		Name->SetVisibility(false);//隐藏战利品名称
	}
}
