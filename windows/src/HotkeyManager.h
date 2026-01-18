#pragma once

#include <windows.h>
#include <functional>

class HotkeyManager {
 public:
  HotkeyManager() = default;
  ~HotkeyManager();

  bool Register(HWND hwnd, UINT id, UINT modifiers, UINT vk, std::function<void()> callback);
  void Unregister();

 private:
  static LRESULT CALLBACK SubclassProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
                                       UINT_PTR id, DWORD_PTR refData);

  HWND m_hwnd = nullptr;
  UINT m_id = 0;
  bool m_registered = false;
  std::function<void()> m_callback;
};
