const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("electronAPI", {
  macro: {
    run: (steps) => ipcRenderer.invoke("macro:run", steps),
    stop: () => ipcRenderer.invoke("macro:stop")
  },
  files: {
    save: (data, suggestedName) => ipcRenderer.invoke("file:save", data, suggestedName),
    open: () => ipcRenderer.invoke("file:open")
  },
  onPlaybackStatus: (handler) => {
    ipcRenderer.removeAllListeners("playback-status");
    ipcRenderer.on("playback-status", (_event, payload) => handler(payload));
  }
});
