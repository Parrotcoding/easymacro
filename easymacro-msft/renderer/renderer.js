const stepsBody = document.getElementById("stepsBody");
const addStepButton = document.getElementById("addStepButton");
const modalBackdrop = document.getElementById("modalBackdrop");
const confirmAdd = document.getElementById("confirmAdd");
const cancelModal = document.getElementById("cancelModal");
const clickPosition = document.getElementById("clickPosition");
const positionInputs = document.getElementById("positionInputs");
const delayMsInput = document.getElementById("delayMs");
const clickButtonSelect = document.getElementById("clickButton");
const playButton = document.getElementById("playButton");
const playbackStatus = document.getElementById("playbackStatus");
const editPanel = document.getElementById("editPanel");
const fileMenuButton = document.getElementById("fileMenuButton");
const fileMenu = document.getElementById("fileMenu");
const fileNameLabel = document.getElementById("fileNameLabel");
const armedToggle = document.getElementById("armedToggle");

const fileNew = document.getElementById("fileNew");
const fileOpen = document.getElementById("fileOpen");
const fileSave = document.getElementById("fileSave");
const fileSaveAs = document.getElementById("fileSaveAs");
const fileNewInline = document.getElementById("fileNewInline");
const fileOpenInline = document.getElementById("fileOpenInline");
const fileSaveInline = document.getElementById("fileSaveInline");
const clearSteps = document.getElementById("clearSteps");

const segmentedButtons = Array.from(document.querySelectorAll(".seg-btn"));

let steps = [];
let selectedId = null;
let isPlaying = false;
let filePath = null;
let fileName = "Untitled.emacro";
let currentStepType = "click";

const formatDelay = (ms) => `${(ms / 1000).toFixed(2)}s`;

const renderEmptyRows = () => {
  const emptyCount = 16;
  for (let i = 0; i < emptyCount; i += 1) {
    const row = document.createElement("div");
    row.className = "table-row";
    row.innerHTML = "<div>&nbsp;</div><div></div><div></div><div></div><div></div>";
    stepsBody.appendChild(row);
  }
};

const renderSteps = () => {
  stepsBody.innerHTML = "";
  if (steps.length === 0) {
    renderEmptyRows();
    return;
  }

  steps.forEach((step, index) => {
    const row = document.createElement("div");
    row.className = "table-row";
    if (step.id === selectedId) {
      row.classList.add("active");
    }
    const location =
      step.type === "wait"
        ? "--"
        : step.position === "custom"
        ? `x: ${step.x} y: ${step.y}`
        : "Current";
    row.innerHTML = `
      <div>${index + 1}</div>
      <div>${step.type === "wait" ? "Wait" : "Click"}</div>
      <div>${step.type === "wait" ? "--" : step.button}</div>
      <div>${location}</div>
      <div>${formatDelay(step.delayMs || 0)}</div>
    `;
    row.addEventListener("click", () => {
      selectedId = step.id;
      renderSteps();
      renderEditPanel();
    });
    stepsBody.appendChild(row);
  });
};

const renderEditPanel = () => {
  const step = steps.find((item) => item.id === selectedId);
  if (!step) {
    editPanel.innerHTML = '<div class="muted">Select a step to edit.</div>';
    return;
  }

  if (step.type === "wait") {
    editPanel.innerHTML = `
      <div class="field">
        <div class="label">Delay (ms)</div>
        <input id="editDelay" type="number" min="0" value="${step.delayMs || 0}" />
      </div>
    `;
    editPanel.querySelector("#editDelay").addEventListener("input", (event) => {
      step.delayMs = Number(event.target.value) || 0;
      renderSteps();
    });
    return;
  }

  editPanel.innerHTML = `
    <div class="field">
      <div class="label">Button</div>
      <select id="editButton">
        <option value="left">Left</option>
        <option value="right">Right</option>
      </select>
    </div>
    <div class="field">
      <div class="label">Position</div>
      <select id="editPosition">
        <option value="current">Current</option>
        <option value="custom">Custom</option>
      </select>
    </div>
    <div class="field inline ${step.position === "custom" ? "" : "hidden"}" id="editPositionInputs">
      <input id="editPosX" type="number" min="0" value="${step.x || 0}" />
      <input id="editPosY" type="number" min="0" value="${step.y || 0}" />
    </div>
    <div class="field">
      <div class="label">Delay (ms)</div>
      <input id="editDelay" type="number" min="0" value="${step.delayMs || 0}" />
    </div>
  `;

  const editButton = editPanel.querySelector("#editButton");
  const editPosition = editPanel.querySelector("#editPosition");
  const editPositionInputs = editPanel.querySelector("#editPositionInputs");
  const editPosX = editPanel.querySelector("#editPosX");
  const editPosY = editPanel.querySelector("#editPosY");
  const editDelay = editPanel.querySelector("#editDelay");

  editButton.value = step.button || "left";
  editPosition.value = step.position || "current";

  editButton.addEventListener("change", (event) => {
    step.button = event.target.value;
    renderSteps();
  });
  editPosition.addEventListener("change", (event) => {
    step.position = event.target.value;
    editPositionInputs.classList.toggle("hidden", step.position !== "custom");
    renderSteps();
  });
  editPosX.addEventListener("input", (event) => {
    step.x = Number(event.target.value) || 0;
    renderSteps();
  });
  editPosY.addEventListener("input", (event) => {
    step.y = Number(event.target.value) || 0;
    renderSteps();
  });
  editDelay.addEventListener("input", (event) => {
    step.delayMs = Number(event.target.value) || 0;
    renderSteps();
  });
};

const openModal = () => {
  modalBackdrop.classList.remove("hidden");
};

const closeModal = () => {
  modalBackdrop.classList.add("hidden");
};

const setStepType = (type) => {
  currentStepType = type;
  segmentedButtons.forEach((button) => {
    button.classList.toggle("active", button.dataset.type === type);
  });
  const clickFields = document.getElementById("clickFields");
  clickFields.classList.toggle("hidden", type !== "click");
};

const addStep = () => {
  const delayMs = Number(delayMsInput.value) || 0;
  if (currentStepType === "wait") {
    steps.push({
      id: crypto.randomUUID(),
      type: "wait",
      delayMs
    });
  } else {
    const position = clickPosition.value;
    const x = Number(document.getElementById("posX").value) || 0;
    const y = Number(document.getElementById("posY").value) || 0;
    steps.push({
      id: crypto.randomUUID(),
      type: "click",
      button: clickButtonSelect.value,
      position,
      x,
      y,
      delayMs
    });
  }
  renderSteps();
  closeModal();
};

const updateFileName = () => {
  fileNameLabel.textContent = fileName;
};

const setFileFromPath = (pathValue) => {
  filePath = pathValue || null;
  if (filePath) {
    const parts = filePath.split(/[/\\\\]/);
    fileName = parts[parts.length - 1];
  } else {
    fileName = "Untitled.emacro";
  }
  updateFileName();
};

const saveFile = async (forceDialog) => {
  const result = await window.electronAPI.files.save(
    { version: 1, steps },
    forceDialog ? fileName : fileName
  );
  if (result?.ok && result.filePath) {
    setFileFromPath(result.filePath);
  }
};

const openFile = async () => {
  const result = await window.electronAPI.files.open();
  if (result?.ok) {
    const data = result.data || {};
    steps = Array.isArray(data.steps) ? data.steps : Array.isArray(data) ? data : [];
    selectedId = null;
    renderSteps();
    renderEditPanel();
    setFileFromPath(result.filePath);
  }
};

const resetFile = () => {
  steps = [];
  selectedId = null;
  renderSteps();
  renderEditPanel();
  setFileFromPath(null);
};

const playMacro = async () => {
  if (isPlaying) {
    await window.electronAPI.macro.stop();
    return;
  }
  if (!armedToggle.checked) {
    playbackStatus.textContent = "Arm playback to start.";
    return;
  }
  if (steps.length === 0) {
    playbackStatus.textContent = "No steps to play.";
    return;
  }
  isPlaying = true;
  playButton.textContent = "Stop";
  await window.electronAPI.macro.run(steps);
};

const toggleMenu = () => {
  fileMenu.classList.toggle("show");
};

document.addEventListener("click", (event) => {
  if (!fileMenuButton.contains(event.target) && !fileMenu.contains(event.target)) {
    fileMenu.classList.remove("show");
  }
});

clickPosition.addEventListener("change", (event) => {
  positionInputs.classList.toggle("hidden", event.target.value !== "custom");
});

segmentedButtons.forEach((button) => {
  button.addEventListener("click", () => {
    setStepType(button.dataset.type);
  });
});

addStepButton.addEventListener("click", openModal);
cancelModal.addEventListener("click", closeModal);
confirmAdd.addEventListener("click", addStep);
playButton.addEventListener("click", playMacro);

fileMenuButton.addEventListener("click", toggleMenu);
fileNew.addEventListener("click", resetFile);
fileOpen.addEventListener("click", openFile);
fileSave.addEventListener("click", () => saveFile(false));
fileSaveAs.addEventListener("click", () => saveFile(true));

fileNewInline.addEventListener("click", resetFile);
fileOpenInline.addEventListener("click", openFile);
fileSaveInline.addEventListener("click", () => saveFile(false));

clearSteps.addEventListener("click", () => {
  steps = [];
  selectedId = null;
  renderSteps();
  renderEditPanel();
});

window.electronAPI.onPlaybackStatus((payload) => {
  if (payload.status === "playing") {
    playbackStatus.textContent = "Playback running";
  } else if (payload.status === "error") {
    playbackStatus.textContent = payload.error || "Playback error";
    isPlaying = false;
    playButton.textContent = "Play";
  } else {
    playbackStatus.textContent = "Playback stopped";
    isPlaying = false;
    playButton.textContent = "Play";
  }
});

window.addEventListener("keydown", async (event) => {
  if (event.ctrlKey && event.shiftKey && event.key.toLowerCase() === "p") {
    await window.electronAPI.macro.stop();
  }
});

setStepType("click");
renderSteps();
renderEditPanel();
updateFileName();
