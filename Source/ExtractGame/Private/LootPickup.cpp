// Fill out your copyright notice in the Description page of Project Settings.


#include "LootPickup.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"
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
	DetectionSphere->SetSphereRadius(500.f);//设置检测范围半径为500
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
		Name->SetText(FText::FromName(LootName));//设置战利品名称
		Name->SetVisibility(true);//显示战利品名称

		Name->SetRelativeLocation(FVector(0.f, 0.f, 200.f));
		Name->SetWorldSize(50.f);
		Name->SetTextRenderColor(FColor::Red);
	}
}

void ALootPickup::OnDetectionEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (Cast<ACharacter>(OtherActor)) {
		Name->SetVisibility(false);//隐藏战利品名称
	}
}