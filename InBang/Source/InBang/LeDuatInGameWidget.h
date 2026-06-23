#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LeDuatInGameWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UPanelWidget;
class UTextBlock;
class UWidget;

UCLASS()
class INBANG_API ULeDuatInGameWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	UFUNCTION(BlueprintCallable, Category = "LeDuat UI")
	void ToggleMissionWindow();

	UFUNCTION(BlueprintCallable, Category = "LeDuat UI")
	void ToggleGodWindow();

	UFUNCTION(BlueprintCallable, Category = "LeDuat UI")
	void ToggleInventoryWindow();

	UFUNCTION(BlueprintCallable, Category = "LeDuat UI")
	void ToggleCollectionWindow();

	UFUNCTION(BlueprintCallable, Category = "LeDuat UI")
	void ToggleGodChat();

	UFUNCTION(BlueprintCallable, Category = "LeDuat UI")
	void ToggleEscMenu();

	UFUNCTION(BlueprintCallable, Category = "LeDuat UI")
	void TriggerDonationToast();

	UFUNCTION(BlueprintCallable, Category = "LeDuat UI")
	void CloseWorkspaceWindows();

private:
	void BuildInterface();
	void BuildMissionHud(UCanvasPanel* Root);
	void BuildPartyHud(UCanvasPanel* Root);
	void BuildTopHud(UCanvasPanel* Root);
	void BuildBottomHud(UCanvasPanel* Root);
	void BuildChatHud(UCanvasPanel* Root);
	void BuildMenuHud(UCanvasPanel* Root);
	void BuildGodChat(UCanvasPanel* Root);
	void BuildDonationToast(UCanvasPanel* Root);
	void BuildMissionWindow(UCanvasPanel* Root);
	void BuildGodWindow(UCanvasPanel* Root);
	void BuildInventoryWindow(UCanvasPanel* Root);
	void BuildCollectionWindow(UCanvasPanel* Root);
	void BuildEscMenu(UCanvasPanel* Root);

	UBorder* MakePanel(const FVector2D& Position, const FVector2D& Size, const FVector2D& Alignment, const FAnchors& Anchors, int32 ZOrder = 0);
	UTextBlock* MakeText(const FString& Text, int32 Size, const FLinearColor& Color, bool bAutoWrap = false);
	void AddTitleRow(UPanelWidget* Parent, const FString& Title, const FString& RightText);
	void AddMissionRow(UPanelWidget* Parent, const FString& Label, const FString& Progress);
	void AddProgressRow(UPanelWidget* Parent, const FString& Label, float Percent, const FString& Value, const FLinearColor& FillColor);
	void AddButtonText(UPanelWidget* Parent, const FString& Label);
	void SetWindowVisible(UWidget* Window, bool bVisible) const;
	void ToggleWindow(UWidget* Window);
	void HideDonationToast();

	UPROPERTY()
	TObjectPtr<UWidget> MissionWindow;

	UPROPERTY()
	TObjectPtr<UWidget> GodWindow;

	UPROPERTY()
	TObjectPtr<UWidget> InventoryWindow;

	UPROPERTY()
	TObjectPtr<UWidget> CollectionWindow;

	UPROPERTY()
	TObjectPtr<UWidget> GodChatPanel;

	UPROPERTY()
	TObjectPtr<UWidget> EscMenu;

	UPROPERTY()
	TObjectPtr<UWidget> DonationToast;

	FTimerHandle DonationToastTimer;

	bool bBuilt = false;
};
