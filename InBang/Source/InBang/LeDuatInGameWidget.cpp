#include "LeDuatInGameWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "TimerManager.h"

namespace
{
	const FLinearColor PanelColor(0.071f, 0.071f, 0.125f, 0.88f);
	const FLinearColor StrongPanelColor(0.098f, 0.090f, 0.169f, 0.96f);
	const FLinearColor SoftPanelColor(0.153f, 0.129f, 0.235f, 0.72f);
	const FLinearColor TextColor(0.965f, 0.949f, 1.0f, 1.0f);
	const FLinearColor MutedColor(0.722f, 0.675f, 0.804f, 1.0f);
	const FLinearColor PurpleColor(0.569f, 0.275f, 1.0f, 1.0f);
	const FLinearColor TealColor(0.094f, 0.816f, 0.761f, 1.0f);
	const FLinearColor GoldColor(0.961f, 0.784f, 0.357f, 1.0f);
	const FLinearColor DangerColor(1.0f, 0.349f, 0.471f, 1.0f);
	const FLinearColor GoodColor(0.380f, 0.886f, 0.580f, 1.0f);

	void SetSlotPadding(UWidget* Widget, const FMargin& Padding)
	{
		if (UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(Widget->Slot))
		{
			VerticalSlot->SetPadding(Padding);
		}
		else if (UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(Widget->Slot))
		{
			HorizontalSlot->SetPadding(Padding);
		}
		else if (UGridSlot* GridSlot = Cast<UGridSlot>(Widget->Slot))
		{
			GridSlot->SetPadding(Padding);
		}
		else if (UUniformGridSlot* UniformGridSlot = Cast<UUniformGridSlot>(Widget->Slot))
		{
			UniformGridSlot->SetHorizontalAlignment(HAlign_Fill);
			UniformGridSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}
}

void ULeDuatInGameWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!bBuilt)
	{
		BuildInterface();
		bBuilt = true;
	}
}

void ULeDuatInGameWidget::BuildInterface()
{
	if (!WidgetTree)
	{
		UE_LOG(LogTemp, Error, TEXT("LeDuatInGameWidget: WidgetTree is null"));
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("LeDuatRoot"));
	WidgetTree->RootWidget = Root;

	BuildMissionHud(Root);
	BuildPartyHud(Root);
	BuildTopHud(Root);
	BuildBottomHud(Root);
	BuildChatHud(Root);
	BuildMenuHud(Root);
	BuildGodChat(Root);
	BuildDonationToast(Root);
	BuildMissionWindow(Root);
	BuildGodWindow(Root);
	BuildInventoryWindow(Root);
	BuildCollectionWindow(Root);
	BuildEscMenu(Root);
}

UBorder* ULeDuatInGameWidget::MakePanel(const FVector2D& Position, const FVector2D& Size, const FVector2D& Alignment, const FAnchors& Anchors, int32 ZOrder)
{
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Panel->SetBrushColor(PanelColor);
	Panel->SetPadding(FMargin(12.0f));

	UCanvasPanel* Root = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	UCanvasPanelSlot* CanvasSlot = Root->AddChildToCanvas(Panel);
	CanvasSlot->SetAnchors(Anchors);
	CanvasSlot->SetAlignment(Alignment);
	CanvasSlot->SetPosition(Position);
	CanvasSlot->SetSize(Size);
	CanvasSlot->SetZOrder(ZOrder);

	return Panel;
}

UTextBlock* ULeDuatInGameWidget::MakeText(const FString& Text, int32 Size, const FLinearColor& Color, bool bAutoWrap)
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TextBlock->SetText(FText::FromString(Text));
	TextBlock->SetColorAndOpacity(FSlateColor(Color));
	TextBlock->SetAutoWrapText(bAutoWrap);
	TextBlock->SetJustification(ETextJustify::Left);

	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = Size;
	TextBlock->SetFont(Font);
	return TextBlock;
}

void ULeDuatInGameWidget::AddTitleRow(UPanelWidget* Parent, const FString& Title, const FString& RightText)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Parent->AddChild(Row);
	SetSlotPadding(Row, FMargin(0.0f, 0.0f, 0.0f, 9.0f));

	UTextBlock* Left = MakeText(Title, 13, TextColor);
	Row->AddChildToHorizontalBox(Left)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UTextBlock* Right = MakeText(RightText, 11, MutedColor);
	Row->AddChildToHorizontalBox(Right)->SetHorizontalAlignment(HAlign_Right);
}

void ULeDuatInGameWidget::AddMissionRow(UPanelWidget* Parent, const FString& Label, const FString& Progress)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Parent->AddChild(Row);
	SetSlotPadding(Row, FMargin(0.0f, 3.0f, 0.0f, 3.0f));

	UTextBlock* LabelText = MakeText(Label, 12, TextColor, true);
	Row->AddChildToHorizontalBox(LabelText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UTextBlock* ProgressText = MakeText(Progress, 12, TealColor);
	Row->AddChildToHorizontalBox(ProgressText)->SetHorizontalAlignment(HAlign_Right);
}

void ULeDuatInGameWidget::AddProgressRow(UPanelWidget* Parent, const FString& Label, float Percent, const FString& Value, const FLinearColor& FillColor)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Parent->AddChild(Row);
	SetSlotPadding(Row, FMargin(0.0f, 3.0f, 0.0f, 3.0f));

	Row->AddChildToHorizontalBox(MakeText(Label, 12, TextColor));

	UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
	Bar->SetPercent(Percent);
	Bar->SetFillColorAndOpacity(FillColor);
	UHorizontalBoxSlot* BarSlot = Row->AddChildToHorizontalBox(Bar);
	BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	BarSlot->SetPadding(FMargin(8.0f, 4.0f));

	Row->AddChildToHorizontalBox(MakeText(Value, 11, MutedColor));
}

void ULeDuatInGameWidget::AddButtonText(UPanelWidget* Parent, const FString& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->SetBackgroundColor(SoftPanelColor);
	Parent->AddChild(Button);
	SetSlotPadding(Button, FMargin(4.0f));

	UTextBlock* Text = MakeText(Label, 12, TextColor, true);
	Text->SetJustification(ETextJustify::Center);
	Button->AddChild(Text);
}

void ULeDuatInGameWidget::BuildMissionHud(UCanvasPanel* Root)
{
	UBorder* Panel = MakePanel(FVector2D(18, 18), FVector2D(292, 132), FVector2D::ZeroVector, FAnchors(0, 0), 1);
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Box);

	AddTitleRow(Box, TEXT("미션"), TEXT("3 / 3 표시"));
	AddMissionRow(Box, TEXT("아누비스의 심판: 정예 적 처치"), TEXT("2 / 3"));
	AddMissionRow(Box, TEXT("이시스의 가호: 아군 회복"), TEXT("78 / 100"));
	AddMissionRow(Box, TEXT("호루스의 시야: 제단 조사"), TEXT("1 / 1"));
}

void ULeDuatInGameWidget::BuildPartyHud(UCanvasPanel* Root)
{
	UBorder* Panel = MakePanel(FVector2D(18, 170), FVector2D(280, 150), FVector2D::ZeroVector, FAnchors(0, 0), 1);
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Box);

	AddTitleRow(Box, TEXT("파티"), TEXT("2명"));
	AddProgressRow(Box, TEXT("LV.7 호루스_초이"), 0.82f, TEXT("82%"), DangerColor);
	Box->AddChild(MakeText(TEXT("버프: 태양 표식 / 출혈 / 신속"), 11, MutedColor));
	AddProgressRow(Box, TEXT("LV.7 바스테트TV"), 0.0f, TEXT("0%"), FLinearColor(0.45f, 0.45f, 0.50f, 1.0f));
	Box->AddChild(MakeText(TEXT("상태: 전투불능 / 보호막 잔여"), 11, MutedColor));
}

void ULeDuatInGameWidget::BuildTopHud(UCanvasPanel* Root)
{
	UBorder* Panel = MakePanel(FVector2D(-18, 18), FVector2D(330, 48), FVector2D(1, 0), FAnchors(1, 0), 1);
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Panel->SetContent(Row);

	UButton* Donation = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Donation->SetBackgroundColor(SoftPanelColor);
	Donation->OnClicked.AddDynamic(this, &ULeDuatInGameWidget::TriggerDonationToast);
	Row->AddChildToHorizontalBox(Donation)->SetPadding(FMargin(0, 0, 8, 0));
	Donation->AddChild(MakeText(TEXT("후원 테스트"), 11, TextColor));

	Row->AddChildToHorizontalBox(MakeText(TEXT("골드 12,450"), 13, GoldColor))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UButton* Esc = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Esc->SetBackgroundColor(SoftPanelColor);
	Esc->OnClicked.AddDynamic(this, &ULeDuatInGameWidget::ToggleEscMenu);
	Row->AddChildToHorizontalBox(Esc);
	Esc->AddChild(MakeText(TEXT("ESC 메뉴"), 11, TextColor));

	UBorder* BuffPanel = MakePanel(FVector2D(-18, 76), FVector2D(235, 56), FVector2D(1, 0), FAnchors(1, 0), 1);
	UVerticalBox* BuffBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	BuffPanel->SetContent(BuffBox);
	AddTitleRow(BuffBox, TEXT("내 버프/디버프"), TEXT("+2"));
	BuffBox->AddChild(MakeText(TEXT("치유 +15%  /  방어 -5%  /  쿨다운 -8%"), 11, MutedColor, true));
}

void ULeDuatInGameWidget::BuildBottomHud(UCanvasPanel* Root)
{
	UBorder* StatPanel = MakePanel(FVector2D(0, -112), FVector2D(760, 96), FVector2D(0.5f, 1), FAnchors(0.5f, 1), 1);
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	StatPanel->SetContent(Box);
	AddProgressRow(Box, TEXT("HP"), 1.0f, TEXT("1,320 / 1,320"), DangerColor);
	AddProgressRow(Box, TEXT("EXP"), 0.42f, TEXT("420 / 1,000"), TealColor);

	UHorizontalBox* Slots = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Box->AddChildToVerticalBox(Slots)->SetPadding(FMargin(0, 8, 0, 0));
	const FString SlotLabels[] = {
		TEXT("Q\n공격 4.2"), TEXT("E\n빛"), TEXT("R\n폭풍 11"), TEXT("C\n방패"),
		TEXT("LV 7"), TEXT("1\n물약 x15"), TEXT("2\n물 x3"), TEXT("3\n주문서 x2"), TEXT("4\n열쇠 x1")
	};
	for (const FString& Label : SlotLabels)
	{
		AddButtonText(Slots, Label);
	}
}

void ULeDuatInGameWidget::BuildChatHud(UCanvasPanel* Root)
{
	UBorder* Panel = MakePanel(FVector2D(18, -18), FVector2D(365, 138), FVector2D(0, 1), FAnchors(0, 1), 1);
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Box);
	AddTitleRow(Box, TEXT("플레이어"), TEXT("파티  시스템"));
	Box->AddChild(MakeText(TEXT("[파티] 호루스_초이: 제단 확인 완료"), 11, TextColor));
	Box->AddChild(MakeText(TEXT("[시스템] 아누비스 미션 진행도 +1"), 11, GoldColor));
	Box->AddChild(MakeText(TEXT("Enter: 메시지 입력창 활성화"), 11, MutedColor));
}

void ULeDuatInGameWidget::BuildMenuHud(UCanvasPanel* Root)
{
	UBorder* Panel = MakePanel(FVector2D(-18, -18), FVector2D(270, 92), FVector2D(1, 1), FAnchors(1, 1), 1);
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Panel->SetContent(Row);

	UButton* Inventory = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Inventory->SetBackgroundColor(SoftPanelColor);
	Inventory->OnClicked.AddDynamic(this, &ULeDuatInGameWidget::ToggleInventoryWindow);
	Row->AddChildToHorizontalBox(Inventory)->SetPadding(FMargin(2));
	Inventory->AddChild(MakeText(TEXT("인벤토리\n[I]"), 11, TextColor));

	UButton* Collection = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Collection->SetBackgroundColor(SoftPanelColor);
	Collection->OnClicked.AddDynamic(this, &ULeDuatInGameWidget::ToggleCollectionWindow);
	Row->AddChildToHorizontalBox(Collection)->SetPadding(FMargin(2));
	Collection->AddChild(MakeText(TEXT("수집품\n[J]"), 11, TextColor));

	UButton* God = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	God->SetBackgroundColor(SoftPanelColor);
	God->OnClicked.AddDynamic(this, &ULeDuatInGameWidget::ToggleGodWindow);
	Row->AddChildToHorizontalBox(God)->SetPadding(FMargin(2));
	God->AddChild(MakeText(TEXT("신 페이지\n[G]"), 11, TextColor));

	UButton* Chat = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Chat->SetBackgroundColor(SoftPanelColor);
	Chat->OnClicked.AddDynamic(this, &ULeDuatInGameWidget::ToggleGodChat);
	Row->AddChildToHorizontalBox(Chat)->SetPadding(FMargin(2));
	Chat->AddChild(MakeText(TEXT("신 채팅\n[B]"), 11, TextColor));
}

void ULeDuatInGameWidget::BuildGodChat(UCanvasPanel* Root)
{
	UBorder* Panel = MakePanel(FVector2D(-18, 152), FVector2D(330, 260), FVector2D(1, 0), FAnchors(1, 0), 2);
	GodChatPanel = Panel;
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Box);
	AddTitleRow(Box, TEXT("신 채팅"), TEXT("B"));
	Box->AddChild(MakeText(TEXT("아누비스: 심판의 저울이 기울고 있다."), 12, TextColor, true));
	Box->AddChild(MakeText(TEXT("이시스: 회복 타이밍을 놓치지 마세요."), 12, GoodColor, true));
	Box->AddChild(MakeText(TEXT("호루스: 서쪽 제단에서 빛이 감지됩니다."), 12, TealColor, true));
	Box->AddChild(MakeText(TEXT("라이브 미션: 정예 적 처치 2 / 3"), 12, GoldColor, true));
	Panel->SetVisibility(ESlateVisibility::Collapsed);
}

void ULeDuatInGameWidget::BuildDonationToast(UCanvasPanel* Root)
{
	UBorder* Panel = MakePanel(FVector2D(0, 150), FVector2D(370, 82), FVector2D(0.5f, 0), FAnchors(0.5f, 0), 5);
	DonationToast = Panel;
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Box);
	AddTitleRow(Box, TEXT("아누비스가 500G 후원"), TEXT("1 / 2"));
	Box->AddChild(MakeText(TEXT("보상 선택 또는 미션 진행도 갱신 알림"), 12, GoldColor, true));
	Panel->SetVisibility(ESlateVisibility::Collapsed);
}

void ULeDuatInGameWidget::BuildMissionWindow(UCanvasPanel* Root)
{
	UBorder* Panel = MakePanel(FVector2D(0, 0), FVector2D(610, 360), FVector2D(0.5f, 0.5f), FAnchors(0.5f, 0.5f), 4);
	MissionWindow = Panel;
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Box);
	AddTitleRow(Box, TEXT("미션 표시 설정"), TEXT("최대 3개"));
	AddMissionRow(Box, TEXT("정예 적 처치 - 아누비스"), TEXT("02:45 / 2-3"));
	AddMissionRow(Box, TEXT("아군 회복 누적 - 이시스"), TEXT("78 / 100"));
	AddMissionRow(Box, TEXT("숨겨진 제단 조사 - 호루스"), TEXT("완료"));
	AddMissionRow(Box, TEXT("동료 1회 부활 - 바스테트"), TEXT("잠금"));
	Box->AddChild(MakeText(TEXT("초기화    취소    적용"), 13, MutedColor));
	Panel->SetVisibility(ESlateVisibility::Collapsed);
}

void ULeDuatInGameWidget::BuildGodWindow(UCanvasPanel* Root)
{
	UBorder* Panel = MakePanel(FVector2D(0, 0), FVector2D(720, 430), FVector2D(0.5f, 0.5f), FAnchors(0.5f, 0.5f), 4);
	GodWindow = Panel;
	UHorizontalBox* Layout = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Panel->SetContent(Layout);

	UVerticalBox* Detail = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Layout->AddChildToHorizontalBox(Detail)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	AddTitleRow(Detail, TEXT("신 페이지"), TEXT("G"));
	Detail->AddChild(MakeText(TEXT("이시스 · Lv.3"), 24, TextColor));
	AddProgressRow(Detail, TEXT("호감도"), 0.62f, TEXT("620 / 1,000"), GoldColor);
	Detail->AddChild(MakeText(TEXT("해금: 치유 증폭"), 13, GoodColor));
	Detail->AddChild(MakeText(TEXT("해금 가능: 토큰 3"), 13, GoldColor));
	Detail->AddChild(MakeText(TEXT("잠금: Lv.4 필요"), 13, MutedColor));

	UVerticalBox* List = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Layout->AddChildToHorizontalBox(List)->SetPadding(FMargin(18, 0, 0, 0));
	List->AddChild(MakeText(TEXT("이시스"), 14, TealColor));
	List->AddChild(MakeText(TEXT("아누비스"), 14, TextColor));
	List->AddChild(MakeText(TEXT("호루스"), 14, TextColor));
	List->AddChild(MakeText(TEXT("바스테트"), 14, TextColor));
	List->AddChild(MakeText(TEXT("토트"), 14, TextColor));
	Panel->SetVisibility(ESlateVisibility::Collapsed);
}

void ULeDuatInGameWidget::BuildInventoryWindow(UCanvasPanel* Root)
{
	UBorder* Panel = MakePanel(FVector2D(0, 0), FVector2D(760, 430), FVector2D(0.5f, 0.5f), FAnchors(0.5f, 0.5f), 4);
	InventoryWindow = Panel;
	UHorizontalBox* Layout = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Panel->SetContent(Layout);

	UVerticalBox* Detail = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Layout->AddChildToHorizontalBox(Detail)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	AddTitleRow(Detail, TEXT("인벤토리"), TEXT("I"));
	Detail->AddChild(MakeText(TEXT("상급 회복 물약"), 22, TextColor));
	Detail->AddChild(MakeText(TEXT("즉시 HP를 35% 회복합니다."), 13, MutedColor, true));
	Detail->AddChild(MakeText(TEXT("사용    사용 불가"), 13, GoldColor));

	UVerticalBox* GridBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Layout->AddChildToHorizontalBox(GridBox)->SetPadding(FMargin(18, 0, 0, 0));
	AddTitleRow(GridBox, TEXT("골드 12,450"), TEXT("정렬: 등급순"));

	UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	GridBox->AddChild(Grid);
	const FString Items[] = { TEXT("물약 x15"), TEXT("정수 x3"), TEXT("약초 x8"), TEXT("주문서 x2"), TEXT("열쇠 x1"), TEXT("항아리 x0"), TEXT("보석 x7"), TEXT("스카라브 x4"), TEXT("깃털 x2") };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Items); ++Index)
	{
		UTextBlock* Item = MakeText(Items[Index], 12, Index == 0 ? GoldColor : TextColor, true);
		UUniformGridSlot* ItemSlot = Grid->AddChildToUniformGrid(Item, Index / 3, Index % 3);
		ItemSlot->SetHorizontalAlignment(HAlign_Fill);
		ItemSlot->SetVerticalAlignment(VAlign_Fill);
		Item->SetMinDesiredWidth(88.0f);
	}
	GridBox->AddChild(MakeText(TEXT("아이템 슬롯을 하단 퀵슬롯으로 드래그해 등록하는 상태"), 11, MutedColor, true));
	Panel->SetVisibility(ESlateVisibility::Collapsed);
}

void ULeDuatInGameWidget::BuildCollectionWindow(UCanvasPanel* Root)
{
	UBorder* Panel = MakePanel(FVector2D(0, 0), FVector2D(700, 390), FVector2D(0.5f, 0.5f), FAnchors(0.5f, 0.5f), 4);
	CollectionWindow = Panel;
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Box);
	AddTitleRow(Box, TEXT("수집품"), TEXT("J"));
	Box->AddChild(MakeText(TEXT("오벨리스크"), 22, TextColor));
	Box->AddChild(MakeText(TEXT("획득 보너스: HP +20"), 13, GoodColor));
	Box->AddChild(MakeText(TEXT("오벨리스크 - HP +20"), 13, TealColor));
	Box->AddChild(MakeText(TEXT("황금 항아리 - 골드 +100"), 13, TextColor));
	Box->AddChild(MakeText(TEXT("파피루스 조각 - 룬석 +1"), 13, TextColor));
	Box->AddChild(MakeText(TEXT("분류: 전체    18 / 36"), 12, MutedColor));
	Panel->SetVisibility(ESlateVisibility::Collapsed);
}

void ULeDuatInGameWidget::BuildEscMenu(UCanvasPanel* Root)
{
	UBorder* Panel = MakePanel(FVector2D(0, 0), FVector2D(360, 310), FVector2D(0.5f, 0.5f), FAnchors(0.5f, 0.5f), 6);
	EscMenu = Panel;
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Box);
	AddTitleRow(Box, TEXT("ESC 메뉴"), TEXT("Esc"));
	AddButtonText(Box, TEXT("설정"));
	AddButtonText(Box, TEXT("조작 가이드"));
	AddButtonText(Box, TEXT("로비로 나가기"));
	AddButtonText(Box, TEXT("게임 종료"));
	Panel->SetVisibility(ESlateVisibility::Collapsed);
}

void ULeDuatInGameWidget::SetWindowVisible(UWidget* Window, bool bVisible) const
{
	if (Window)
	{
		Window->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void ULeDuatInGameWidget::ToggleWindow(UWidget* Window)
{
	if (!Window)
	{
		return;
	}

	const bool bShouldShow = Window->GetVisibility() == ESlateVisibility::Collapsed || Window->GetVisibility() == ESlateVisibility::Hidden;
	CloseWorkspaceWindows();
	SetWindowVisible(Window, bShouldShow);
}

void ULeDuatInGameWidget::ToggleMissionWindow()
{
	ToggleWindow(MissionWindow);
}

void ULeDuatInGameWidget::ToggleGodWindow()
{
	ToggleWindow(GodWindow);
}

void ULeDuatInGameWidget::ToggleInventoryWindow()
{
	ToggleWindow(InventoryWindow);
}

void ULeDuatInGameWidget::ToggleCollectionWindow()
{
	ToggleWindow(CollectionWindow);
}

void ULeDuatInGameWidget::ToggleGodChat()
{
	if (GodChatPanel)
	{
		const bool bShouldShow = GodChatPanel->GetVisibility() != ESlateVisibility::Visible;
		SetWindowVisible(GodChatPanel, bShouldShow);
	}
}

void ULeDuatInGameWidget::ToggleEscMenu()
{
	if (EscMenu)
	{
		const bool bShouldShow = EscMenu->GetVisibility() != ESlateVisibility::Visible;
		SetWindowVisible(EscMenu, bShouldShow);
	}
}

void ULeDuatInGameWidget::TriggerDonationToast()
{
	SetWindowVisible(DonationToast, true);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DonationToastTimer);
		FTimerDelegate HideDelegate;
		HideDelegate.BindUObject(this, &ULeDuatInGameWidget::HideDonationToast);
		World->GetTimerManager().SetTimer(DonationToastTimer, HideDelegate, 2.5f, false);
	}
}

void ULeDuatInGameWidget::CloseWorkspaceWindows()
{
	SetWindowVisible(MissionWindow, false);
	SetWindowVisible(GodWindow, false);
	SetWindowVisible(InventoryWindow, false);
	SetWindowVisible(CollectionWindow, false);
}

void ULeDuatInGameWidget::HideDonationToast()
{
	SetWindowVisible(DonationToast, false);
}
