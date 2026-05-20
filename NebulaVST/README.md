# NEBULA VST3 — Virtual Analog Synthesizer

Полноценный виртуально-аналоговый синтезатор как **VST3-плагин** для FL Studio 21+ / Ableton / Reaper / Cubase.

**Архитектура:** C++ DSP-ядро + GUI на HTML/CSS/JS через Microsoft Edge **WebView2**. Без JUCE, без iPlug2, без сторонних плагин-фреймворков — только Steinberg VST3 SDK и WebView2 SDK.

```
┌─────────────────────────────────────────────────┐
│  WebView2 (HTML/CSS/JS UI — synth.html)         │  ← знакомый интерфейс
└────────────┬───────────────────▲────────────────┘
             │  postMessage      │  параметры/scope
             ▼                   │
┌─────────────────────────────────────────────────┐
│  C++ Controller (NebulaController)              │
│  - параметры VST3                               │
│  - host <-> WebView мост                        │
└────────────┬───────────────────▲────────────────┘
             │ IPC параметров     │ MIDI/Audio
             ▼                   │
┌─────────────────────────────────────────────────┐
│  C++ Processor (NebulaProcessor)                │
│  - polyphonic voice manager (16)                │
│  - PolyBLEP oscillators, ADSR, SVF filter,      │
│    waveshaper, chorus, delay, FDN reverb        │
└─────────────────────────────────────────────────┘
```

## Что внутри

- 16-голосная полифония, voice stealing
- 2 PolyBLEP-осциллятора (saw/square/triangle/sine), без алиасинга
- Sub-oscillator + white noise
- State-variable filter (LP/HP/BP/Notch), нелинейный (`tanh`) drive
- 2× ADSR (filter & amp), экспоненциальные сегменты
- LFO с маршрутизацией на pitch/filter/amp/PW
- FX: chorus → delay → FDN reverb → master
- Параметры автоматизируются хостом (VST3 IParameterChanges)
- Сохранение состояния в проекте FL Studio
- GUI — тот же `synth.html` из веб-версии, переиспользован 1:1 (с правкой Web Audio → WebView messaging)

## Сборка (Windows 11, Visual Studio 2022)

### 1. Установите зависимости

```powershell
# 1. VS 2022 c "Desktop development with C++" workload (у вас уже есть)
# 2. CMake 3.20+ (включён в VS 2022)
# 3. Git
```

### 2. Скачайте SDK

```powershell
cd NebulaVST
git clone --recursive https://github.com/steinbergmedia/vst3sdk.git external/vst3sdk
```

WebView2 SDK подтянется через NuGet автоматически (`packages.config`).

### 3. Соберите

```powershell
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

На выходе: `build/Release/Nebula.vst3` (это папка-bundle).

### 4. Установите в систему

Скопируйте `Nebula.vst3` в:
```
C:\Program Files\Common Files\VST3\
```

### 5. В FL Studio

`Options → Manage plugins → Find plugins` → Nebula появится в списке.

## Файлы проекта

```
NebulaVST/
├── CMakeLists.txt                  ← сборка
├── README.md
├── src/
│   ├── plugin/
│   │   ├── NebulaFactory.cpp       ← VST3 entry points
│   │   ├── NebulaProcessor.h/.cpp  ← аудио + MIDI
│   │   ├── NebulaController.h/.cpp ← параметры + GUI
│   │   ├── NebulaEditor.h/.cpp     ← WebView2 host
│   │   └── ParameterIds.h          ← общие ID параметров
│   ├── dsp/
│   │   ├── Voice.h/.cpp            ← один голос
│   │   ├── PolyBlepOsc.h           ← антиалиасинговый осциллятор
│   │   ├── SvfFilter.h             ← state-variable filter
│   │   ├── Envelope.h              ← ADSR
│   │   ├── Lfo.h
│   │   ├── Chorus.h
│   │   ├── Delay.h
│   │   └── Reverb.h                ← FDN
│   └── gui/
│       └── synth.html              ← переиспользованный UI
└── external/
    └── vst3sdk/                    ← склонируете сами
```
