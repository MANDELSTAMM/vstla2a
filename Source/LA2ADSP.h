// LA2ADSP.h
// Поведенческая DSP-модель электрооптического компрессора типа LA-2A.
//
// Это не транзисторный/ламповый circuit-sim, а тщательно подобранная
// поведенческая модель ключевых физических особенностей T4-опто-ячейки:
//  - Feedback-топология (детектор слушает ВЫХОД, а не вход)
//  - Двухстадийный релиз: ~50% восстановления за ~60мс, остаток —
//    за время от ~0.8 до ~5с, зависящее от того, как долго и сильно
//    "светилась" ячейка (программно-зависимая память, ключевая черта опто-компрессоров)
//  - Мягкое колено передаточной кривой, имитирующее нелинейность LDR
//  - Compress / Limit меняют порог чувствительности, колено и соотношение
//  - Ламповый выходной каскад с мягким несимметричным насыщением

#pragma once
#include <cmath>
#include <algorithm>

namespace alx
{

inline float ampToDb (float a) noexcept { return 20.0f * std::log10 (std::max (a, 1.0e-6f)); }
inline float dbToAmp (float d) noexcept { return std::pow (10.0f, d / 20.0f); }

//==============================================================================
/** Один общий опто-детектор. Для стерео используется один экземпляр на пару
    каналов (linked detection), как принято для студийных стерео-версий
    опто-компрессоров — это исключает "перетекание" стереобазы при компрессии.
*/
class OptoDetector
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
    }

    void reset() noexcept
    {
        envFast = 0.0f;
        envSlow = 0.0f;
        driveHistory = 0.0f;
        lastGRdb = 0.0f;
    }

    /** detLevel       - линейный уровень детектируемого (выходного) сигнала
        peakReduction  - 0..1, ручка Peak Reduction
        mode           - 0 = Compress, 1 = Limit
        возвращает мгновенно сглаженное по баллистике значение gain reduction в дБ (>=0)
    */
    float process (float detLevel, float peakReduction, int mode) noexcept
    {
        const float levelDb = ampToDb (detLevel);

        // Peak Reduction добавляет "драйв" в цепь детектора - в реальном LA-2A
        // именно так работает эта ручка: она не задаёт жёсткий threshold,
        // а регулирует, сколько сигнала попадает на засветку ячейки.
        const float driveDb     = peakReduction * 46.0f;
        const float sidechainDb = levelDb + driveDb;

        const float thresholdDb = (mode == 1) ? -34.0f : -22.0f;
        const float ratio       = (mode == 1) ?  12.0f  :   3.0f;
        const float kneeDb      = (mode == 1) ?   6.0f  :  16.0f;

        const float instGRdb = softKneeGR (sidechainDb, thresholdDb, ratio, kneeDb);

        // --- Баллистика опто-ячейки ---
        // Атака: ячейка засвечивается быстро (несколько мс), чуть быстрее
        // при сильных пиках - лёгкая level-dependency для реализма.
        const float attackMs    = 2.0f + 3.0f * (1.0f - peakReduction);
        const float attackCoeff = timeToCoeff (attackMs * 0.001f);

        // Релиз, стадия 1: первые ~50% восстановления за ~60мс
        const float relFastCoeff = timeToCoeff (0.060f);

        // Релиз, стадия 2: программно-зависимый "хвост" фосфоресценции ячейки -
        // чем дольше и сильнее давили компрессию, тем медленнее восстановление.
        const float slowSeconds  = 0.8f + 4.2f * driveHistory;
        const float relSlowCoeff = timeToCoeff (slowSeconds);

        // Память о недавней "истории засветки" (нормирована 0..1)
        const float histTarget = std::clamp (instGRdb / 20.0f, 0.0f, 1.0f);
        const float histCoeff  = timeToCoeff (3.0f);
        driveHistory += (histTarget - driveHistory) * histCoeff;

        if (instGRdb > envFast) envFast += (instGRdb - envFast) * attackCoeff;
        else                    envFast += (instGRdb - envFast) * relFastCoeff;

        if (instGRdb > envSlow) envSlow += (instGRdb - envSlow) * attackCoeff;
        else                    envSlow += (instGRdb - envSlow) * relSlowCoeff;

        const float applied = std::max (0.0f, 0.5f * envFast + 0.5f * envSlow);
        lastGRdb = applied;
        return applied;
    }

    float getLastGRdb() const noexcept { return lastGRdb; }

private:
    double sr = 44100.0;
    float envFast = 0.0f, envSlow = 0.0f, driveHistory = 0.0f, lastGRdb = 0.0f;

    float timeToCoeff (float seconds) const noexcept
    {
        seconds = std::max (seconds, 0.0001f);
        return 1.0f - std::exp (-1.0f / (seconds * (float) sr));
    }

    static float softKneeGR (float inputDb, float thresholdDb, float ratio, float kneeDb) noexcept
    {
        const float over     = inputDb - thresholdDb;
        const float halfKnee = kneeDb * 0.5f;
        const float slope    = 1.0f - 1.0f / ratio;

        float gr;
        if (over <= -halfKnee)
            gr = 0.0f;
        else if (over >= halfKnee)
            gr = over * slope;
        else
        {
            const float t = over + halfKnee;
            gr = (t * t) / (2.0f * kneeDb) * slope;
        }
        return std::max (0.0f, gr);
    }
};

//==============================================================================
/** Имитация лампового выходного каскада (тёплая мягкая сатурация
    с лёгкой чётно-гармонической асимметрией, как у 12AX7/6AQ5). */
class TubeStage
{
public:
    float process (float x) const noexcept
    {
        const float drive = 1.35f;
        const float z = x * drive;
        // Несимметричное мягкое ограничение: чуть больше насыщения на положительной
        // полуволне, как у однотактного лампового каскада.
        const float asym = 0.06f;
        const float shaped = std::tanh (z + asym * z * z);
        return shaped / std::tanh (drive); // приблизительно unity-gain на малых уровнях
    }
};

} // namespace alx
