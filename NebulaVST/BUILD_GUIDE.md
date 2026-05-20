# Сборка NEBULA VST3 — пошагово

## Что нужно
- Windows 11
- Visual Studio 2022 с workload **Desktop development with C++** ✅ (у вас уже есть)
- Git
- ~2 ГБ свободного места

## Шаг 1. Получите проект

```powershell
cd C:\dev      # или куда удобно
# Скопируйте сюда папку NebulaVST из workspace
cd NebulaVST
```

## Шаг 2. Скачайте VST3 SDK

```powershell
git clone --recursive https://github.com/steinbergmedia/vst3sdk.git external/vst3sdk
```

⚠️ Steinberg требует регистрации, но репозиторий публичный для GPLv3-сборок.
Если будете распространять — прочитайте лицензию (GPLv3 или Steinberg Dual License).

## Шаг 3. Скачайте WebView2 SDK

```powershell
mkdir external
cd external
nuget install Microsoft.Web.WebView2 -Version 1.0.2210.55 -OutputDirectory .
ren Microsoft.Web.WebView2.1.0.2210.55 webview2
cd ..
```

Если нет `nuget.exe` — скачайте с https://www.nuget.org/downloads и положите в PATH.

Альтернатива: откройте `Nebula.sln` (после генерации CMake), правый клик
по проекту → **Manage NuGet Packages** → найдите `Microsoft.Web.WebView2`.

## Шаг 4. Убедитесь, что WebView2 Runtime установлен

В Windows 11 он уже есть (часть Edge). Проверить:
`Settings → Apps → Installed apps → "Microsoft Edge WebView2 Runtime"`.

Если нет: https://developer.microsoft.com/microsoft-edge/webview2/

## Шаг 5. Соберите

```powershell
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 ^
      -DWEBVIEW2_DIR=external/webview2
cmake --build build --config Release
```

После успешной сборки:
```
build\VST3\Release\Nebula.vst3\Contents\x86_64-win\Nebula.vst3
```
(VST3 SDK кладёт результат внутрь bundle-папки)

## Шаг 6. Установите в систему

Скопируйте всю папку `Nebula.vst3` (это bundle, не один файл!) в:
```
C:\Program Files\Common Files\VST3\
```

## Шаг 7. Подключите в FL Studio

1. `Options → Manage plugins`
2. Убедитесь, что путь `C:\Program Files\Common Files\VST3` есть в списке
3. Нажмите `Find plugins` (Find more / Verify)
4. После поиска: правый клик по найденному "Nebula" → **Add to plugin database**
5. Откройте Channel rack → `+` → Nebula → играйте на пианоролле!

## Если что-то не работает

| Проблема | Решение |
|----------|---------|
| `cannot find WebView2.h` | Проверьте `WEBVIEW2_DIR`, должен указывать на папку с `build/native/include/WebView2.h` |
| `WebView2LoaderStatic.lib not found` | В новых SDK файл называется `WebView2LoaderStatic.lib` или `WebView2Loader.dll.lib` — поправьте путь в CMakeLists.txt |
| FL Studio не видит плагин | Запустите `Nebula.vst3` через VST3 Validator (`vst3sdk/public.sdk/samples/vst-hosting/validator`) — он покажет ошибки регистрации |
| Окно плагина чёрное | Не установлен WebView2 Runtime. Установите. |
| Звук с щелчками | Уменьшите Sample buffer в FL Studio до 256 семплов |

## Архитектура (зачем нужно столько файлов)

Web-версия `synth.html` помещалась в один файл, потому что Web Audio дал ей готовый DSP-движок и автоматическую полифонию. Здесь же нам пришлось:

1. **`PolyBlepOsc.h`** — антиалиасинговый осциллятор (в Web Audio это `OscillatorNode`)
2. **`SvfFilter.h`** — стабильный state-variable filter (в Web Audio — `BiquadFilterNode`)
3. **`Envelope.h`** — экспоненциальный ADSR (в Web Audio — `linearRampToValueAtTime`)
4. **`Voice.h/.cpp`** — менеджер одного голоса (Web Audio управляет нодами сам)
5. **`Chorus/Delay/Reverb.h`** — каждый эффект руками
6. **`NebulaProcessor`** — обработка `process()` колбэка, MIDI, voice stealing
7. **`NebulaController`** — параметры для хоста с правильной денормализацией
8. **`NebulaEditor`** — мост HTML ↔ C++ через WebView2 + `postMessage`
9. **`NebulaFactory`** — регистрация UID и точек входа для DAW
10. **`CMakeLists.txt`** — сборка под VS 2022 + кодогенерация (HTML → header)

Это и есть ответ на ваш вопрос «почему VST так сложно по сравнению с вебом» — на одну и ту же функциональность здесь ~10 раз больше кода, потому что Web Audio API закрывает примерно 80% этой работы из коробки. 🙂
