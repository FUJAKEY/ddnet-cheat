// Простой тест синтаксиса TAS системы
#include "fujix_tas.h"

// Минимальная проверка компиляции
void TestTasSyntax()
{
    // Проверяем что структуры корректно определены
    CFujixTas::SStateSnapshot snapshot;
    CFujixTas::SEntry entry;
    
    snapshot.m_Tick = 0;
    snapshot.m_PosX = 0.0f;
    entry.m_Tick = 0;
    
    // Проверяем что методы доступны
    CFujixTas tas;
    tas.IsRecording();
    tas.IsPlaying();
    tas.IsTesting();
}
#include "fujix_pathfinding.cpp"
int main() { return 0; }

