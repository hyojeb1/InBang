#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "TalentFaceAnimInstance.generated.h"

struct FTalentFaceAnimInstanceProxy : public FAnimInstanceProxy
{
	using FAnimInstanceProxy::FAnimInstanceProxy;

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode) override;

private:
	TMap<FName, float> CurvesWrittenAfterGraph;
};

UCLASS(Blueprintable)
class INBANG_API UTalentFaceAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	void SetCurvesWrittenAfterGraph(TMap<FName, float> Curves) { CurvesWrittenAfterGraph = MoveTemp(Curves); }
	const TMap<FName, float>& GetCurvesWrittenAfterGraph() const { return CurvesWrittenAfterGraph; }

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

private:
	TMap<FName, float> CurvesWrittenAfterGraph;
};
