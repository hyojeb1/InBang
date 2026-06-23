#include "CalculatorEditor.h"

#include "CalculatorEngine.h"
#include "CalculatorOperation.h"
#include "Framework/Docking/TabManager.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FCalculatorEditorModule"

namespace CalculatorEditor
{
	static const FName TabName(TEXT("CalculatorEditorTab"));

	static FText GetOperationText(ECalculatorOperation Operation)
	{
		switch (Operation)
		{
		case ECalculatorOperation::Add:
			return LOCTEXT("AddOperation", "Add");
		case ECalculatorOperation::Subtract:
			return LOCTEXT("SubtractOperation", "Subtract");
		case ECalculatorOperation::Multiply:
			return LOCTEXT("MultiplyOperation", "Multiply");
		case ECalculatorOperation::Divide:
			return LOCTEXT("DivideOperation", "Divide");
		default:
			return LOCTEXT("UnknownOperation", "Unknown");
		}
	}
}

class SCalculatorWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCalculatorWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		OperationOptions.Add(MakeShared<ECalculatorOperation>(ECalculatorOperation::Add));
		OperationOptions.Add(MakeShared<ECalculatorOperation>(ECalculatorOperation::Subtract));
		OperationOptions.Add(MakeShared<ECalculatorOperation>(ECalculatorOperation::Multiply));
		OperationOptions.Add(MakeShared<ECalculatorOperation>(ECalculatorOperation::Divide));
		SelectedOperation = OperationOptions[0];

		ChildSlot
		[
			SNew(SBox)
			.Padding(16.0f)
			.MinDesiredWidth(360.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 12.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CalculatorTitle", "Calculator"))
					.TextStyle(FAppStyle::Get(), "HeadingMedium")
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SGridPanel)

					+ SGridPanel::Slot(0, 0)
					.Padding(0.0f, 0.0f, 8.0f, 8.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("InputALabel", "A"))
					]

					+ SGridPanel::Slot(1, 0)
					.Padding(0.0f, 0.0f, 0.0f, 8.0f)
					[
						SAssignNew(InputA, SEditableTextBox)
						.Text(FText::FromString(TEXT("0")))
					]

					+ SGridPanel::Slot(0, 1)
					.Padding(0.0f, 0.0f, 8.0f, 8.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("InputBLabel", "B"))
					]

					+ SGridPanel::Slot(1, 1)
					.Padding(0.0f, 0.0f, 0.0f, 8.0f)
					[
						SAssignNew(InputB, SEditableTextBox)
						.Text(FText::FromString(TEXT("0")))
					]

					+ SGridPanel::Slot(0, 2)
					.Padding(0.0f, 0.0f, 8.0f, 8.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("OperationLabel", "Operation"))
					]

					+ SGridPanel::Slot(1, 2)
					.Padding(0.0f, 0.0f, 0.0f, 8.0f)
					[
						SNew(SComboBox<TSharedPtr<ECalculatorOperation>>)
						.OptionsSource(&OperationOptions)
						.InitiallySelectedItem(SelectedOperation)
						.OnGenerateWidget(this, &SCalculatorWidget::GenerateOperationWidget)
						.OnSelectionChanged(this, &SCalculatorWidget::OnOperationChanged)
						[
							SNew(STextBlock)
							.Text(this, &SCalculatorWidget::GetSelectedOperationText)
						]
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 8.0f, 0.0f, 12.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("CalculateButton", "Calculate"))
					.OnClicked(this, &SCalculatorWidget::OnCalculateClicked)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(ResultText, STextBlock)
					.Text(LOCTEXT("ResultPlaceholder", "Result:"))
				]
			]
		];
	}

private:
	TSharedRef<SWidget> GenerateOperationWidget(TSharedPtr<ECalculatorOperation> Operation) const
	{
		return SNew(STextBlock)
			.Text(Operation.IsValid()
				? CalculatorEditor::GetOperationText(*Operation)
				: LOCTEXT("InvalidOperation", "Invalid"));
	}

	void OnOperationChanged(TSharedPtr<ECalculatorOperation> NewOperation, ESelectInfo::Type SelectInfo)
	{
		if (NewOperation.IsValid())
		{
			SelectedOperation = NewOperation;
		}
	}

	FText GetSelectedOperationText() const
	{
		return SelectedOperation.IsValid()
			? CalculatorEditor::GetOperationText(*SelectedOperation)
			: LOCTEXT("NoOperationSelected", "Select operation");
	}

	FReply OnCalculateClicked()
	{
		const double A = FCString::Atod(*InputA->GetText().ToString());
		const double B = FCString::Atod(*InputB->GetText().ToString());
		double Result = 0.0;
		FString ErrorMessage;

		const bool bSucceeded = FCalculatorEngine::Calculate(
			SelectedOperation.IsValid() ? *SelectedOperation : ECalculatorOperation::Add,
			A,
			B,
			Result,
			ErrorMessage);

		ResultText->SetText(bSucceeded
			? FText::Format(LOCTEXT("ResultFormat", "Result: {0}"), FText::AsNumber(Result))
			: FText::Format(LOCTEXT("ErrorFormat", "Error: {0}"), FText::FromString(ErrorMessage)));

		return FReply::Handled();
	}

	TSharedPtr<SEditableTextBox> InputA;
	TSharedPtr<SEditableTextBox> InputB;
	TSharedPtr<STextBlock> ResultText;
	TArray<TSharedPtr<ECalculatorOperation>> OperationOptions;
	TSharedPtr<ECalculatorOperation> SelectedOperation;
};

void FCalculatorEditorModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		CalculatorEditor::TabName,
		FOnSpawnTab::CreateRaw(this, &FCalculatorEditorModule::SpawnCalculatorTab))
		.SetDisplayName(LOCTEXT("CalculatorTabTitle", "Calculator"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FCalculatorEditorModule::RegisterMenus));
}

void FCalculatorEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(CalculatorEditor::TabName);
}

void FCalculatorEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));
	FToolMenuSection& Section = ToolsMenu->FindOrAddSection(TEXT("Calculator"));
	Section.AddMenuEntry(
		TEXT("OpenCalculator"),
		LOCTEXT("OpenCalculatorLabel", "Calculator"),
		LOCTEXT("OpenCalculatorToolTip", "Open the calculator test tab."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FCalculatorEditorModule::OpenCalculatorTab)));
}

void FCalculatorEditorModule::OpenCalculatorTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(CalculatorEditor::TabName);
}

TSharedRef<SDockTab> FCalculatorEditorModule::SpawnCalculatorTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SCalculatorWidget)
		];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCalculatorEditorModule, CalculatorEditor)
