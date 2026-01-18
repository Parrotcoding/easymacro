const { app, BrowserWindow, ipcMain, dialog } = require("electron");
const path = require("path");
const fs = require("fs");
const { execFile } = require("child_process");

let mainWindow = null;
let isPlaying = false;
let stopRequested = false;

const createWindow = () => {
  mainWindow = new BrowserWindow({
    width: 1200,
    height: 760,
    minWidth: 980,
    minHeight: 640,
    backgroundColor: "#f4f4f4",
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
      contextIsolation: true,
      nodeIntegration: false
    }
  });

  mainWindow.loadFile(path.join(__dirname, "renderer", "index.html"));
};

const sleep = (ms) =>
  new Promise((resolve) => {
    setTimeout(resolve, ms);
  });

const runWindowsClick = (step) =>
  new Promise((resolve, reject) => {
    const button = step.button === "right" ? "right" : "left";
    const downFlag = button === "right" ? 0x0008 : 0x0002;
    const upFlag = button === "right" ? 0x0010 : 0x0004;
    const usePosition = step.position === "custom";
    const x = Number.isFinite(step.x) ? Math.round(step.x) : 0;
    const y = Number.isFinite(step.y) ? Math.round(step.y) : 0;

    const script = `
Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public class MouseSim {
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int X, int Y);
  [DllImport("user32.dll")] public static extern void mouse_event(int dwFlags, int dx, int dy, int cButtons, int dwExtraInfo);
}
"@
${usePosition ? `[MouseSim]::SetCursorPos(${x}, ${y}) | Out-Null` : ""}
[MouseSim]::mouse_event(${downFlag}, 0, 0, 0, 0)
[MouseSim]::mouse_event(${upFlag}, 0, 0, 0, 0)
`;

    execFile(
      "powershell.exe",
      ["-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", script],
      (error) => {
        if (error) {
          reject(error);
          return;
        }
        resolve();
      }
    );
  });

const runMacro = async (steps) => {
  if (isPlaying) {
    return { ok: false, error: "Already playing" };
  }
  if (process.platform !== "win32") {
    return { ok: false, error: "Macro playback requires Windows." };
  }
  isPlaying = true;
  stopRequested = false;
  mainWindow?.webContents.send("playback-status", { status: "playing" });
  for (let i = 0; i < steps.length; i += 1) {
    if (stopRequested) {
      break;
    }
    const step = steps[i];
    mainWindow?.webContents.send("playback-status", {
      status: "playing",
      currentIndex: i
    });
    if (step.type === "wait") {
      await sleep(step.delayMs || 0);
    } else if (step.type === "click") {
      try {
        await runWindowsClick(step);
      } catch (error) {
        mainWindow?.webContents.send("playback-status", {
          status: "error",
          error: error.message || "Click failed"
        });
        isPlaying = false;
        return { ok: false, error: error.message || "Click failed" };
      }
      await sleep(step.delayMs || 0);
    }
  }
  isPlaying = false;
  stopRequested = false;
  mainWindow?.webContents.send("playback-status", { status: "stopped" });
  return { ok: true };
};

ipcMain.handle("macro:run", async (_event, steps) => runMacro(steps || []));
ipcMain.handle("macro:stop", async () => {
  stopRequested = true;
  return { ok: true };
});

ipcMain.handle("file:save", async (_event, data, suggestedName) => {
  const result = await dialog.showSaveDialog(mainWindow, {
    title: "Save Macro",
    defaultPath: suggestedName || "macro.emacro",
    filters: [{ name: "EasyMacro Files", extensions: ["emacro"] }]
  });
  if (result.canceled || !result.filePath) {
    return { ok: false };
  }
  fs.writeFileSync(result.filePath, JSON.stringify(data, null, 2), "utf8");
  return { ok: true, filePath: result.filePath };
});

ipcMain.handle("file:open", async () => {
  const result = await dialog.showOpenDialog(mainWindow, {
    title: "Open Macro",
    filters: [{ name: "EasyMacro Files", extensions: ["emacro"] }],
    properties: ["openFile"]
  });
  if (result.canceled || !result.filePaths?.[0]) {
    return { ok: false };
  }
  const filePath = result.filePaths[0];
  const raw = fs.readFileSync(filePath, "utf8");
  const data = JSON.parse(raw);
  return { ok: true, filePath, data };
});

app.whenReady().then(createWindow);

app.on("window-all-closed", () => {
  if (process.platform !== "darwin") {
    app.quit();
  }
});
