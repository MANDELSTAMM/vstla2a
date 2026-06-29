# ALX-LA-2A

Поведенческая DSP-модель электрооптического компрессора типа LA-2A,
оформленная как VST3-плагин (JUCE).

## Что внутри (Source/LA2ADSP.h)

- **Feedback-топология** — детектор слушает выходной сигнал, а не входной,
  как в оригинальной схеме T4-ячейки.
- **Двухстадийный program-dependent release** — ~50% восстановления за
  ~60 мс (стадия 1) и остаток за время от ~0.8 до ~5 с (стадия 2),
  зависящее от "истории засветки" ячейки — чем дольше и сильнее давили
  компрессию, тем медленнее релиз. Это ключевая, узнаваемая черта именно
  опто-компрессоров, в отличие от VCA/FET с фиксированными атакой/релизом.
- **Peak Reduction** работает не как threshold, а как добавочный "драйв"
  в сайдчейн — ровно так это сделано в оригинале.
- **Compress / Limit** меняют порог чувствительности, ширину колена и
  ratio (мягкий ~3:1 против жёсткого ~12:1).
- **Стерео-линкованная детекция** — общий детектор для L/R, чтобы
  компрессия не "гуляла" по стереобазе.
- **Ламповый выходной каскад** — мягкая несимметричная сатурация (tanh +
  чётная гармоника) для тёплого характера, плюс makeup gain.
- Винтажный GUI с анимированным VU-метром (режимы GR / +4 / +8),
  бакелитовыми ручками и рокерными переключателями Power и Compress/Limit.

Это честная behavioural-модель ключевых физических особенностей T4-ячейки,
а не транзисторный circuit-sim — такой уровень точности соответствует тому,
как сделаны почти все профессиональные плагин-эмуляции LA-2A.

## Готовый билд

В архиве `ALX-LA-2A-VST3-Linux.zip` лежит уже собранный VST3 для **Linux**
(x86_64), собранный и проверенный прямо в этом окружении (экспортирует
`GetPluginFactory`/`ModuleEntry`, валидная структура модуля). Подходит для
REAPER/Bitwig/Ardour/Carla на Linux — просто положите папку
`ALX-LA-2A.vst3` в `~/.vst3/`.

## Сборка под Windows — без установки Visual Studio (рекомендуемый способ)

Кросс-компиляция VST3 под Windows из Linux через MinGW официально не
поддерживается JUCE 8 (там стоит жёсткая проверка `#error "MinGW is not
supported"`) — это сознательное решение разработчиков JUCE, потому что такие
сборки давали труднообъяснимые краши внутри DAW. Поэтому правильный путь —
собрать настоящим MSVC-компилятором. Сделать это можно бесплатно и без
установки Visual Studio на свой компьютер — через GitHub Actions:

1. Создайте новый репозиторий на github.com (можно приватный, это бесплатно).
2. Залейте туда содержимое этого архива (включая папку `.github/workflows`).
3. Откройте вкладку **Actions** в репозитории — сборка запустится
   автоматически (или нажмите "Run workflow" вручную).
4. Через 3–5 минут в результатах прогона появится артефакт
   **ALX-LA-2A-Windows-VST3** — скачайте его, внутри будет настоящий,
   собранный MSVC `ALX-LA-2A.vst3` для Windows x64.
5. Распакуйте папку `ALX-LA-2A.vst3` в
   `C:\Program Files\Common Files\VST3\`.

Workflow уже лежит в `.github/workflows/build.yml` — ничего настраивать не
нужно, просто запушить проект.

## Сборка под Windows локально (если уже есть Visual Studio)

Для Windows и macOS нужен бинарник, собранный непосредственно на этой ОС
(кросс-компиляция VST3 с GUI не делается в одну сторону). Соберите сами —
это пара команд:

```bash
git clone --depth 1 https://github.com/juce-framework/JUCE.git
# положите папку JUCE рядом с CMakeLists.txt (или сделайте symlink/copy
# так, чтобы JUCE оказался в ./JUCE относительно CMakeLists.txt)

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
```

- **Windows**: нужен Visual Studio 2022 (Desktop C++ workload) либо
  `cmake -B build -G "Visual Studio 17 2022"`.
- **macOS**: нужен Xcode + command line tools, можно `cmake -B build -G Xcode`.
- **Linux**: нужны `cmake ninja-build build-essential libasound2-dev
  libfreetype6-dev libfontconfig1-dev libx11-dev libxext-dev libxrandr-dev
  libxinerama-dev libxcursor-dev libxcomposite-dev libcurl4-openssl-dev
  libgl1-mesa-dev`.

Готовый плагин появится в:
`build/ALX_LA_2A_artefacts/Release/VST3/ALX-LA-2A.vst3`

Скопируйте эту папку в системную папку VST3-плагинов вашей DAW
(`C:\Program Files\Common Files\VST3` на Windows,
`~/Library/Audio/Plug-Ins/VST3` на macOS).
