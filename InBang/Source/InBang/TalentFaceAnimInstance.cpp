#include "TalentFaceAnimInstance.h"

void FTalentFaceAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	CurvesWrittenAfterGraph = CastChecked<UTalentFaceAnimInstance>(InAnimInstance)->GetCurvesWrittenAfterGraph();
}

bool FTalentFaceAnimInstanceProxy::Evaluate_WithRoot(FPoseContext& Output, FAnimNode_Base* InRootNode)
{
	EvaluateAnimationNode_WithRoot(Output, InRootNode);
	for (const TPair<FName, float>& Curve : CurvesWrittenAfterGraph)
	{
		Output.Curve.Set(Curve.Key, Curve.Value);
	}
	return true;
}

FAnimInstanceProxy* UTalentFaceAnimInstance::CreateAnimInstanceProxy()
{
	return new FTalentFaceAnimInstanceProxy(this);
}
