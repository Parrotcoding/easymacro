#include "pch.h"
#include "MainWindow.xaml.h"
#include "MacroAction.h"

#include <winrt/Microsoft.UI.Interop.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Pickers.h>

#include <filesystem>
#include <iomanip>
#include <sstream>
#include <chrono>

#include <shobjidl.h>

using namespace winrt;
using namespace winrt::Microsoft::UI;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Media;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Pickers;
using namespace winrt::Windows::Data::Json;

namespace {
std::wstring GetComboTag(ComboBox const& combo) {
  auto item = combo.SelectedItem().as<ComboBoxItem>();
  if (!item) {
    return L"";
  }
  if (item.Tag()) {
    return unbox_value<hstring>(item.Tag());
  }
  return L"";
}

bool IsRadioChecked(RadioButton const& button) {
  auto value = button.IsChecked();
  return value && value.Value();
}

bool TryParseDouble(std::wstring const& text, double& value, bool allowZero = true) {
  try {
    size_t idx = 0;
    value = std::stod(text, &idx);
    if (idx != text.size()) {
      return false;
    }
    if (allowZero) {
      return value >= 0.0;
    }
    return value > 0.0;
  } catch (...) {
    return false;
  }
}

Color RowBackgroundColor(bool isSelected, int index) {
  if (isSelected) {
    return ColorHelper::FromArgb(255, 229, 229, 229);
  }
  return (index % 2 == 0) ? ColorHelper::FromArgb(255, 245, 245, 245)
                          : ColorHelper::FromArgb(255, 250, 250, 250);
}
}  // namespace

namespace winrt::EasyMacroWin::implementation {
MainWindow::MainWindow() {
  InitializeComponent();

  auto windowNative = this->try_as<winrt::Microsoft::UI::Xaml::IWindowNative>();
  if (windowNative) {
    windowNative->get_WindowHandle(&m_hwnd);
  }

  InitializeDefaults();
  RenderSteps();
  UpdateEditPanel();
  UpdateFileName();

  if (m_hwnd) {
    m_hotkey.Register(m_hwnd, 1, MOD_CONTROL | MOD_ALT | MOD_SHIFT, 'P', [this]() { OnPanicHotkey(); });
  }
}

void MainWindow::InitializeDefaults() {
  LoopToggle().IsOn(true);
  AddButtonCombo().SelectedIndex(0);
  AddPositionCombo().SelectedIndex(0);
  AddXBox().Text(L"0");
  AddYBox().Text(L"0");
  AddDelayBox().Text(L"10");
  EditKindCombo().SelectedIndex(0);
  EditXBox().Text(L"0");
  EditYBox().Text(L"0");
  EditDelayBox().Text(L"0");
}

void MainWindow::UpdateStatus(std::wstring_view status) {
  StatusText().Text(status);
}

void MainWindow::UpdateFileName() {
  FileNameText().Text(m_fileName);
}

void MainWindow::RenderSteps() {
  StepsPanel().Children().Clear();

  if (m_actions.empty()) {
    for (int i = 0; i < 16; ++i) {
      Border spacer;
      spacer.Height(28);
      spacer.CornerRadius(CornerRadiusHelper::FromUniformRadius(6));
      spacer.Background(SolidColorBrush(RowBackgroundColor(false, i)));
      StepsPanel().Children().Append(spacer);
    }
    PlayButton().IsEnabled(false);
    return;
  }

  PlayButton().IsEnabled(true);
  for (size_t i = 0; i < m_actions.size(); ++i) {
    const auto& action = m_actions[i];

    Grid row;
    row.Padding(ThicknessHelper::FromLengths(8, 6, 8, 6));
    row.CornerRadius(CornerRadiusHelper::FromUniformRadius(6));
    row.Background(SolidColorBrush(RowBackgroundColor(static_cast<int>(i) == m_selectedIndex, static_cast<int>(i))));
    row.Tag(box_value(static_cast<int>(i)));

    ColumnDefinition colIndex;
    colIndex.Width(GridLengthHelper::FromPixels(40));
    row.ColumnDefinitions().Append(colIndex);
    ColumnDefinition colButton;
    colButton.Width(GridLengthHelper::FromPixels(100));
    row.ColumnDefinitions().Append(colButton);
    ColumnDefinition colLocation;
    colLocation.Width(GridLengthHelper::FromPixels(140));
    row.ColumnDefinitions().Append(colLocation);
    ColumnDefinition colSpacer;
    colSpacer.Width(GridLengthHelper::FromValueAndType(1, GridUnitType::Star));
    row.ColumnDefinitions().Append(colSpacer);
    ColumnDefinition colDelay;
    colDelay.Width(GridLengthHelper::FromPixels(90));
    row.ColumnDefinitions().Append(colDelay);

    TextBlock indexText;
    indexText.Text(std::to_wstring(i + 1));
    row.Children().Append(indexText);

    TextBlock kindText;
    kindText.Text(KindLabel(action.kind));
    Grid::SetColumn(kindText, 1);
    row.Children().Append(kindText);

    TextBlock locationText;
    locationText.Text(LocationLabel(action));
    Grid::SetColumn(locationText, 2);
    row.Children().Append(locationText);

    TextBlock delayText;
    delayText.Text(FormatDelay(action.delay));
    delayText.HorizontalAlignment(HorizontalAlignment::Right);
    Grid::SetColumn(delayText, 4);
    row.Children().Append(delayText);

    row.Tapped([this, i](auto&&, auto&&) { SelectRow(i); });
    StepsPanel().Children().Append(row);
  }
}

void MainWindow::SelectRow(size_t index) {
  if (index >= m_actions.size()) {
    return;
  }
  m_selectedIndex = static_cast<int>(index);
  RenderSteps();
  UpdateEditPanel();
}

void MainWindow::UpdateEditPanel() {
  if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_actions.size())) {
    EditEmptyText().Visibility(Visibility::Visible);
    EditFormPanel().Visibility(Visibility::Collapsed);
    return;
  }

  const auto& action = m_actions[static_cast<size_t>(m_selectedIndex)];
  EditEmptyText().Visibility(Visibility::Collapsed);
  EditFormPanel().Visibility(Visibility::Visible);

  switch (action.kind) {
    case ActionKind::LeftClick:
      EditKindCombo().SelectedIndex(0);
      break;
    case ActionKind::RightClick:
      EditKindCombo().SelectedIndex(1);
      break;
    case ActionKind::OtherClick:
      EditKindCombo().SelectedIndex(2);
      break;
    case ActionKind::Wait:
      EditKindCombo().SelectedIndex(3);
      break;
  }

  EditXBox().Text(std::to_wstring(static_cast<int>(action.x)));
  EditYBox().Text(std::to_wstring(static_cast<int>(action.y)));
  EditDelayBox().Text(std::to_wstring(action.delay));
  EditXYRow().Visibility(action.kind == ActionKind::Wait ? Visibility::Collapsed : Visibility::Visible);
}

void MainWindow::AddStepButton_Click(IInspectable const&, RoutedEventArgs const&) {
  AddTypeClickRadio().IsChecked(true);
  AddButtonCombo().SelectedIndex(0);
  AddPositionCombo().SelectedIndex(0);
  AddXBox().Text(L"0");
  AddYBox().Text(L"0");
  AddDelayBox().Text(L"10");

  AddClickFields().Visibility(Visibility::Visible);
  AddWaitFields().Visibility(Visibility::Collapsed);
  AddCustomPosition().Visibility(Visibility::Collapsed);

  AddStepDialog().XamlRoot(Content().XamlRoot());
  AddStepDialog().ShowAsync();
}

void MainWindow::AddTypeRadio_Checked(IInspectable const&, RoutedEventArgs const&) {
  bool isClick = IsRadioChecked(AddTypeClickRadio());
  AddClickFields().Visibility(isClick ? Visibility::Visible : Visibility::Collapsed);
  AddWaitFields().Visibility(isClick ? Visibility::Collapsed : Visibility::Visible);
}

void MainWindow::AddPositionCombo_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&) {
  auto tag = GetComboTag(AddPositionCombo());
  AddCustomPosition().Visibility(tag == L"custom" ? Visibility::Visible : Visibility::Collapsed);
}

void MainWindow::AddStepDialog_PrimaryButtonClick(ContentDialog const&,
                                                  ContentDialogButtonClickEventArgs const& args) {
  MacroAction action{};
  std::wstring error;
  if (!TryResolveAddStep(action, error)) {
    UpdateStatus(error);
    args.Cancel(true);
    return;
  }

  m_actions.push_back(action);
  m_selectedIndex = static_cast<int>(m_actions.size() - 1);
  UpdateStatus(L"Added step");
  RenderSteps();
  UpdateEditPanel();
}

bool MainWindow::TryResolveAddStep(MacroAction& action, std::wstring& error) {
  action.id = GenerateGuidString();
  if (IsRadioChecked(AddTypeWaitRadio())) {
    double delay = 0.0;
    if (!TryParseDouble(AddDelayBox().Text().c_str(), delay)) {
      error = L"Enter a valid wait time";
      return false;
    }
    action.kind = ActionKind::Wait;
    action.delay = delay;
    action.x = 0.0;
    action.y = 0.0;
    return true;
  }

  auto buttonTag = GetComboTag(AddButtonCombo());
  if (buttonTag == L"rightClick") {
    action.kind = ActionKind::RightClick;
  } else if (buttonTag == L"otherClick") {
    action.kind = ActionKind::OtherClick;
  } else {
    action.kind = ActionKind::LeftClick;
  }

  auto positionTag = GetComboTag(AddPositionCombo());
  if (positionTag == L"center") {
    int cx = GetSystemMetrics(SM_CXSCREEN);
    int cy = GetSystemMetrics(SM_CYSCREEN);
    action.x = cx / 2.0;
    action.y = cy / 2.0;
  } else if (positionTag == L"mouse") {
    POINT pt{};
    GetCursorPos(&pt);
    action.x = static_cast<double>(pt.x);
    action.y = static_cast<double>(pt.y);
  } else if (positionTag == L"custom") {
    double x = 0.0;
    double y = 0.0;
    if (!TryParseDouble(AddXBox().Text().c_str(), x) || !TryParseDouble(AddYBox().Text().c_str(), y)) {
      error = L"Enter valid coordinates";
      return false;
    }
    action.x = x;
    action.y = y;
  } else {
    error = L"Select a position";
    return false;
  }

  action.delay = 0.0;
  return true;
}

void MainWindow::EditKindCombo_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&) {
  auto tag = GetComboTag(EditKindCombo());
  EditXYRow().Visibility(tag == L"wait" ? Visibility::Collapsed : Visibility::Visible);
}

void MainWindow::ApplyEditButton_Click(IInspectable const&, RoutedEventArgs const&) {
  if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_actions.size())) {
    UpdateStatus(L"Select a step to edit");
    return;
  }

  auto tag = GetComboTag(EditKindCombo());
  ActionKind kind = KindFromString(winrt::to_string(tag));

  double delay = 0.0;
  if (!TryParseDouble(EditDelayBox().Text().c_str(), delay)) {
    UpdateStatus(L"Delay must be 0 or greater");
    return;
  }

  double x = 0.0;
  double y = 0.0;
  if (kind != ActionKind::Wait) {
    if (!TryParseDouble(EditXBox().Text().c_str(), x) || !TryParseDouble(EditYBox().Text().c_str(), y)) {
      UpdateStatus(L"Enter valid X and Y");
      return;
    }
  }

  auto& action = m_actions[static_cast<size_t>(m_selectedIndex)];
  action.kind = kind;
  action.delay = delay;
  action.x = x;
  action.y = y;

  UpdateStatus(L"Updated step");
  RenderSteps();
  UpdateEditPanel();
}

void MainWindow::DeleteStepButton_Click(IInspectable const&, RoutedEventArgs const&) {
  if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_actions.size())) {
    UpdateStatus(L"Select a step to delete");
    return;
  }
  m_actions.erase(m_actions.begin() + m_selectedIndex);
  m_selectedIndex = -1;
  UpdateStatus(L"Deleted step");
  RenderSteps();
  UpdateEditPanel();
}

void MainWindow::ClearStepsButton_Click(IInspectable const&, RoutedEventArgs const&) {
  m_actions.clear();
  m_selectedIndex = -1;
  UpdateStatus(L"Ready");
  RenderSteps();
  UpdateEditPanel();
}

void MainWindow::PlayButton_Click(IInspectable const&, RoutedEventArgs const&) {
  if (m_isPlaying) {
    StopPlayback(L"Playback stopped");
    return;
  }
  StartPlayback();
}

void MainWindow::StartPlayback() {
  if (m_actions.empty()) {
    UpdateStatus(L"No steps to play");
    return;
  }
  if (m_isPlaying) {
    return;
  }

  m_stopRequested = false;
  m_isPlaying = true;
  PlayButton().Content(box_value(L"Stop"));

  const bool loop = LoopToggle().IsOn();
  UpdateStatus(loop ? L"Looping macro" : L"Playing macro");

  auto actionsCopy = m_actions;
  auto dispatcher = DispatcherQueue();
  auto weak = get_weak();

  std::thread worker([this, actionsCopy, loop, dispatcher, weak]() mutable {
    do {
      for (const auto& action : actionsCopy) {
        if (m_stopRequested.load()) {
          break;
        }
        if (action.delay > 0) {
          std::this_thread::sleep_for(std::chrono::duration<double>(action.delay));
        }
        if (m_stopRequested.load()) {
          break;
        }
        if (action.kind != ActionKind::Wait) {
          PerformAction(action);
        }
      }
    } while (loop && !m_stopRequested.load());

    bool stopped = m_stopRequested.load();
    dispatcher.TryEnqueue([weak, loop, stopped]() {
      if (auto self = weak.get()) {
        self->m_isPlaying = false;
        self->PlayButton().Content(box_value(L"Play"));
        if (!stopped) {
          self->UpdateStatus(loop ? L"Loop finished" : L"Finished playback");
        }
      }
    });
  });

  m_playbackThread = std::move(worker);
  m_playbackThread.detach();
}

void MainWindow::StopPlayback(std::wstring_view statusOverride) {
  m_stopRequested = true;
  m_isPlaying = false;
  PlayButton().Content(box_value(L"Play"));
  UpdateStatus(statusOverride);
}

void MainWindow::PerformAction(const MacroAction& action) {
  int x = static_cast<int>(action.x);
  int y = static_cast<int>(action.y);
  SetCursorPos(x, y);

  DWORD downFlag = MOUSEEVENTF_LEFTDOWN;
  DWORD upFlag = MOUSEEVENTF_LEFTUP;
  if (action.kind == ActionKind::RightClick) {
    downFlag = MOUSEEVENTF_RIGHTDOWN;
    upFlag = MOUSEEVENTF_RIGHTUP;
  } else if (action.kind == ActionKind::OtherClick) {
    downFlag = MOUSEEVENTF_MIDDLEDOWN;
    upFlag = MOUSEEVENTF_MIDDLEUP;
  }

  INPUT inputs[2]{};
  inputs[0].type = INPUT_MOUSE;
  inputs[0].mi.dwFlags = downFlag;
  inputs[1].type = INPUT_MOUSE;
  inputs[1].mi.dwFlags = upFlag;
  SendInput(2, inputs, sizeof(INPUT));
}

void MainWindow::OnPanicHotkey() {
  DispatcherQueue().TryEnqueue([this]() {
    if (m_isPlaying) {
      StopPlayback(L"Emergency stop");
      return;
    }
    if (m_actions.empty()) {
      UpdateStatus(L"Nothing to play");
      return;
    }
    StartPlayback();
    UpdateStatus(L"Panic start");
  });
}

void MainWindow::FileNew_Click(IInspectable const&, RoutedEventArgs const&) {
  m_actions.clear();
  m_selectedIndex = -1;
  m_currentFilePath.clear();
  m_fileName = L"Untitled.emacro";
  UpdateStatus(L"Ready");
  UpdateFileName();
  RenderSteps();
  UpdateEditPanel();
}

winrt::fire_and_forget MainWindow::OpenFileAsync() {
  auto lifetime = get_strong();

  FileOpenPicker picker;
  picker.FileTypeFilter().Append(L".emacro");
  auto initializeWithWindow = picker.as<IInitializeWithWindow>();
  initializeWithWindow->Initialize(m_hwnd);

  StorageFile file = co_await picker.PickSingleFileAsync();
  if (!file) {
    co_return;
  }

  auto text = co_await FileIO::ReadTextAsync(file);
  try {
    JsonArray array = JsonArray::Parse(text);
    m_actions = ParseActionsFromJson(array);
    m_selectedIndex = -1;
    m_currentFilePath = file.Path().c_str();
    m_fileName = file.Name().c_str();
    UpdateStatus(L"Loaded steps");
    UpdateFileName();
    RenderSteps();
    UpdateEditPanel();
  } catch (...) {
    UpdateStatus(L"Couldn't open file");
  }
}

winrt::fire_and_forget MainWindow::SaveFileAsync(bool asNew) {
  auto lifetime = get_strong();

  StorageFile file{ nullptr };
  try {
    if (asNew || m_currentFilePath.empty()) {
      FileSavePicker picker;
      picker.FileTypeChoices().Insert(L"EasyMacro Files", single_threaded_vector<hstring>({ L".emacro" }));
      picker.SuggestedFileName(m_fileName.empty() ? L"Untitled.emacro" : m_fileName);
      picker.DefaultFileExtension(L".emacro");

      auto initializeWithWindow = picker.as<IInitializeWithWindow>();
      initializeWithWindow->Initialize(m_hwnd);

      file = co_await picker.PickSaveFileAsync();
      if (!file) {
        co_return;
      }
    } else {
      file = co_await StorageFile::GetFileFromPathAsync(m_currentFilePath);
    }

    JsonArray array = SerializeActionsToJson(m_actions);
    co_await FileIO::WriteTextAsync(file, array.Stringify());
    m_currentFilePath = file.Path().c_str();
    m_fileName = file.Name().c_str();
    UpdateStatus(L"Saved macro");
    UpdateFileName();
  } catch (...) {
    UpdateStatus(L"Failed to save");
  }
}

void MainWindow::FileOpen_Click(IInspectable const&, RoutedEventArgs const&) {
  OpenFileAsync();
}

void MainWindow::FileSave_Click(IInspectable const&, RoutedEventArgs const&) {
  SaveFileAsync(false);
}

void MainWindow::FileSaveAs_Click(IInspectable const&, RoutedEventArgs const&) {
  SaveFileAsync(true);
}
}  // namespace winrt::EasyMacroWin::implementation
