#include "pch.h"
#include "HotkeyManager.h"

#include <commctrl.h>

HotkeyManager::~HotkeyManager() {
  Unregister();
}

bool HotkeyManager::Register(HWND hwnd, UINT id, UINT modifiers, UINT vk, std::function<void()> callback) {
  Unregister();
  if (!hwnd) {
    return false;
  }

  m_hwnd = hwnd;
  m_id = id;
  m_callback = std::move(callback);

  if (!SetWindowSubclass(hwnd, &HotkeyManager::SubclassProc, 1, reinterpret_cast<DWORD_PTR>(this))) {
    m_hwnd = nullptr;
    m_id = 0;
    return false;
  }

  if (!RegisterHotKey(hwnd, id, modifiers, vk)) {
    RemoveWindowSubclass(hwnd, &HotkeyManager::SubclassProc, 1);
    m_hwnd = nullptr;
    m_id = 0;
    return false;
  }

  m_registered = true;
  return true;
}

void HotkeyManager::Unregister() {
  if (m_hwnd && m_registered) {
    UnregisterHotKey(m_hwnd, m_id);
    RemoveWindowSubclass(m_hwnd, &HotkeyManager::SubclassProc, 1);
  }
  m_registered = false;
  m_hwnd = nullptr;
  m_id = 0;
  m_callback = nullptr;
}

LRESULT CALLBACK HotkeyManager::SubclassProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
                                             UINT_PTR id, DWORD_PTR refData) {
  auto* manager = reinterpret_cast<HotkeyManager*>(refData);
  if (message == WM_HOTKEY && manager && wparam == manager->m_id) {
    if (manager->m_callback) {
      manager->m_callback();
    }
    return 0;
  }
  return DefSubclassProc(hwnd, message, wparam, lparam);
}
