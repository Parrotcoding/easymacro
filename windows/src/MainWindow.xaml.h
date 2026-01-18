#pragma once

#include "MainWindow.g.h"
#include "MacroAction.h"
#include "HotkeyManager.h"

#include <atomic>
#include <thread>
#include <vector>

namespace winrt::EasyMacroWin::implementation {
struct MainWindow : MainWindowT<MainWindow> {
  MainWindow();

  void AddStepButton_Click(winrt::Windows::Foundation::IInspectable const&,
                           winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
  void PlayButton_Click(winrt::Windows::Foundation::IInspectable const&,
                        winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
  void FileNew_Click(winrt::Windows::Foundation::IInspectable const&,
                     winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
  void FileOpen_Click(winrt::Windows::Foundation::IInspectable const&,
                      winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
  void FileSave_Click(winrt::Windows::Foundation::IInspectable const&,
                      winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
  void FileSaveAs_Click(winrt::Windows::Foundation::IInspectable const&,
                        winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
  void ClearStepsButton_Click(winrt::Windows::Foundation::IInspectable const&,
                              winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
  void AddStepDialog_PrimaryButtonClick(winrt::Microsoft::UI::Xaml::Controls::ContentDialog const&,
                                        winrt::Microsoft::UI::Xaml::Controls::ContentDialogButtonClickEventArgs const&);
  void AddTypeRadio_Checked(winrt::Windows::Foundation::IInspectable const&,
                            winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
  void AddPositionCombo_SelectionChanged(winrt::Windows::Foundation::IInspectable const&,
                                         winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
  void EditKindCombo_SelectionChanged(winrt::Windows::Foundation::IInspectable const&,
                                      winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
  void ApplyEditButton_Click(winrt::Windows::Foundation::IInspectable const&,
                             winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
  void DeleteStepButton_Click(winrt::Windows::Foundation::IInspectable const&,
                              winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);

 private:
  void InitializeDefaults();
  void RenderSteps();
  void UpdateEditPanel();
  void UpdateFileName();
  void UpdateStatus(std::wstring_view status);
  void StartPlayback();
  void StopPlayback(std::wstring_view statusOverride = L"Playback stopped");
  void PerformAction(const MacroAction& action);
  void OnPanicHotkey();
  void SelectRow(size_t index);
  bool TryResolveAddStep(MacroAction& action, std::wstring& error);
  winrt::fire_and_forget OpenFileAsync();
  winrt::fire_and_forget SaveFileAsync(bool asNew);

  HWND m_hwnd = nullptr;
  HotkeyManager m_hotkey{};
  std::vector<MacroAction> m_actions{};
  int m_selectedIndex = -1;
  bool m_isPlaying = false;
  std::atomic<bool> m_stopRequested{false};
  std::thread m_playbackThread{};
  std::wstring m_currentFilePath{};
  std::wstring m_fileName = L"Untitled.emacro";
};
}

namespace winrt::EasyMacroWin::factory_implementation {
struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {};
}
