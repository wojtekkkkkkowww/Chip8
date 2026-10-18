import { GAMES } from "./games.js";

const gameMenu = document.querySelector("#game-menu");
const gameList = document.querySelector("#game-list");
const gameView = document.querySelector("#game-view");
const gameTitle = document.querySelector("#game-title");
const gameDescription = document.querySelector("#game-description");
const gameControls = document.querySelector("#game-controls");
const screen = document.querySelector("#screen");
const context = screen.getContext("2d", { alpha: false });
const backButton = document.querySelector("#back");
const pauseButton = document.querySelector("#pause");
const resetButton = document.querySelector("#reset");
const status = document.querySelector("#status");

const SCREEN_WIDTH = 64;
const SCREEN_HEIGHT = 32;
const TIMER_INTERVAL_MS = 1000 / 60;
const CHIP8_KEYS = {
  Digit1: 0x1, Digit2: 0x2, Digit3: 0x3, Digit4: 0xC,
  KeyQ: 0x4, KeyW: 0x5, KeyE: 0x6, KeyR: 0xD,
  KeyA: 0x7, KeyS: 0x8, KeyD: 0x9, KeyF: 0xE,
  KeyZ: 0xA, KeyX: 0x0, KeyC: 0xB, KeyV: 0xF,
  ArrowUp: 0x5, ArrowLeft: 0x7, ArrowDown: 0x8, ArrowRight: 0x9,
};

const frame = context.createImageData(SCREEN_WIDTH, SCREEN_HEIGHT);
const pressedKeys = new Map();
const pressedKeyCounts = new Map();
const gameCards = [];
let emulator;
let currentGame;
let running = false;
let cycleAccumulator = 0;
let timerAccumulator = 0;
let previousTimestamp;

function makeGameCard(game) {
  const card = document.createElement("button");
  card.type = "button";
  card.className = "game-card";
  card.disabled = true;

  const image = document.createElement("img");
  image.src = game.preview;
  image.alt = `${game.title} gameplay preview`;
  image.loading = "lazy";
  card.append(image);

  const body = document.createElement("span");
  body.className = "game-card-body";

  const title = document.createElement("span");
  title.className = "game-card-title";
  title.textContent = game.title;
  body.append(title);

  const description = document.createElement("span");
  description.className = "game-card-description";
  description.textContent = game.description;
  body.append(description);

  const controls = document.createElement("span");
  controls.className = "game-card-controls";
  const controlsLabel = document.createElement("strong");
  controlsLabel.textContent = "Controls: ";
  controls.append(controlsLabel, document.createTextNode(game.controls));
  body.append(controls);

  const play = document.createElement("span");
  play.className = "game-card-play";
  play.textContent = "Play game →";
  body.append(play);

  card.append(body);
  gameList.append(card);
  gameCards.push(card);
  card.addEventListener("click", () => startGame(game.id));
}

function renderMenu() {
  for (const game of GAMES) makeGameCard(game);
}

function renderScreen() {
  const pixels = emulator.screenPixels();
  for (let pixel = 0; pixel < pixels.length; pixel += 1) {
    const color = pixels[pixel] ? currentGame.foreground : currentGame.background;
    const offset = pixel * 4;
    frame.data[offset] = (color >> 16) & 0xff;
    frame.data[offset + 1] = (color >> 8) & 0xff;
    frame.data[offset + 2] = color & 0xff;
    frame.data[offset + 3] = 0xff;
  }
  context.putImageData(frame, 0, 0);
}

function setKeyPressed(key, pressed) {
  const count = pressedKeyCounts.get(key) ?? 0;
  if (pressed && count === 0) emulator.setKey(key, true);
  if (!pressed && count === 1) emulator.setKey(key, false);

  const nextCount = count + (pressed ? 1 : -1);
  if (nextCount > 0) pressedKeyCounts.set(key, nextCount);
  else pressedKeyCounts.delete(key);
}

function releaseAllKeys() {
  for (const key of pressedKeyCounts.keys()) emulator.setKey(key, false);
  pressedKeys.clear();
  pressedKeyCounts.clear();
}

function startGame(gameId) {
  const game = GAMES.find((entry) => entry.id === gameId);
  if (!game) return;

  releaseAllKeys();
  emulator.loadRom(game.rom);
  currentGame = game;
  cycleAccumulator = 0;
  timerAccumulator = 0;
  previousTimestamp = undefined;
  running = true;

  gameTitle.textContent = game.title;
  gameDescription.textContent = game.description;
  gameControls.textContent = game.controls;
  pauseButton.textContent = "Pause";
  gameMenu.hidden = true;
  gameView.hidden = false;
  status.textContent = `Running ${game.title}.`;
  renderScreen();
}

function returnToMenu() {
  running = false;
  releaseAllKeys();
  currentGame = undefined;
  gameView.hidden = true;
  gameMenu.hidden = false;
  status.textContent = "";
}

function animate(timestamp) {
  if (previousTimestamp === undefined) previousTimestamp = timestamp;
  const elapsed = Math.min(timestamp - previousTimestamp, 100);
  previousTimestamp = timestamp;

  if (running) {
    cycleAccumulator += elapsed * 60 / 1000 * currentGame.instructionsPerFrame * currentGame.speed;
    timerAccumulator += elapsed * currentGame.speed;

    try {
      const cycles = Math.floor(cycleAccumulator);
      cycleAccumulator -= cycles;
      emulator.runCycles(cycles);

      while (timerAccumulator >= TIMER_INTERVAL_MS) {
        emulator.tickTimers();
        timerAccumulator -= TIMER_INTERVAL_MS;
      }
      renderScreen();
    } catch (error) {
      running = false;
      pauseButton.textContent = "Resume";
      status.textContent = "The ROM stopped with an emulator error. Check the browser console.";
      console.error(error);
    }
  }

  window.requestAnimationFrame(animate);
}

renderMenu();

try {
  const { default: createChip8Module } = await import("./chip8.js");
  const module = await createChip8Module();
  emulator = new module.Chip8();
  gameCards.forEach((card) => { card.disabled = false; });
  pauseButton.disabled = false;
  resetButton.disabled = false;
  status.textContent = "";

  backButton.addEventListener("click", returnToMenu);
  pauseButton.addEventListener("click", () => {
    running = !running;
    pauseButton.textContent = running ? "Pause" : "Resume";
    status.textContent = running ? `Running ${currentGame.title}.` : "Paused.";
  });
  resetButton.addEventListener("click", () => startGame(currentGame.id));

  window.addEventListener("keydown", (event) => {
    if (!currentGame || (event.target instanceof Element && event.target.closest("input, select"))) return;
    const key = CHIP8_KEYS[event.code];
    if (key === undefined) return;
    event.preventDefault();
    if (pressedKeys.has(event.code)) return;
    pressedKeys.set(event.code, key);
    setKeyPressed(key, true);
  });

  window.addEventListener("keyup", (event) => {
    const key = pressedKeys.get(event.code);
    if (key === undefined) return;
    event.preventDefault();
    pressedKeys.delete(event.code);
    setKeyPressed(key, false);
  });

  window.addEventListener("blur", releaseAllKeys);
  window.addEventListener("pagehide", (event) => {
    if (!event.persisted) {
      releaseAllKeys();
      emulator.delete();
    }
  });
  window.requestAnimationFrame(animate);
} catch (error) {
  status.textContent = "Failed to start the emulator. Check the browser console and make sure all page files are present.";
  console.error(error);
}
