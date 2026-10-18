const stepButton = document.querySelector("#step");
const resetButton = document.querySelector("#reset");
const status = document.querySelector("#status");
const pc = document.querySelector("#pc");
const v0 = document.querySelector("#v0");
const stepsOutput = document.querySelector("#steps");

// 6000: V0 = 0; 7001: V0 += 1; 1202: jump back to the addition.
const demoRom = new Uint8Array([0x60, 0x00, 0x70, 0x01, 0x12, 0x02]);

try {
  const { default: createChip8Module } = await import("./chip8.js");
  const module = await createChip8Module();
  const emulator = new module.Chip8();
  let steps = 0;

  function render() {
    pc.textContent = `0x${emulator.getProgramCounter().toString(16).toUpperCase().padStart(3, "0")}`;
    v0.textContent = `0x${emulator.getV0().toString(16).toUpperCase().padStart(2, "0")}`;
    stepsOutput.textContent = steps;
  }

  function reset() {
    emulator.loadProgram(demoRom);
    steps = 0;
    render();
  }

  stepButton.addEventListener("click", () => {
    emulator.makeStep();
    steps += 1;
    render();
  });
  resetButton.addEventListener("click", reset);

  reset();
  stepButton.disabled = false;
  resetButton.disabled = false;
  status.textContent = "Ready. Execute the first instruction.";
  window.addEventListener("pagehide", (event) => {
    if (!event.persisted) emulator.delete();
  });
} catch (error) {
  status.textContent = "Failed to start the emulator. Check the browser console and make sure all page files are present.";
  console.error(error);
}
