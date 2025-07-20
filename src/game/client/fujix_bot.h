#ifndef GAME_CLIENT_FUJIX_BOT_H
#define GAME_CLIENT_FUJIX_BOT_H

#include <base/vmath.h>

class CGameClient;
class CCollision;

/**
 * FUJIX Geros Bot - Умный AI для предотвращения попадания в фриз тайлы
 * 
 * Функциональность:
 * - Предсказание траектории движения игрока
 * - Детекция фриз тайлов в направлении движения  
 * - Учет пинга и сетевой задержки
 * - Автоматическая остановка на безопасной дистанции
 * - Блокировка опасных направлений движения
 */
class CFujixBot
{
private:
	CGameClient *m_pGameClient;
	CCollision *m_pCollision;
	
	// Настройки бота
	float m_SafeDistance;        // Безопасная дистанция от фриз тайлов (в тайлах)
	float m_PingCompensation;    // Дополнительная дистанция для компенсации пинга
	int m_PredictionTicks;       // Количество тиков для предсказания
	
	// Текущее состояние
	vec2 m_PlayerPos;           // Текущая позиция игрока
	vec2 m_PlayerVel;           // Текущая скорость игрока
	vec2 m_PredictedPos;        // Предсказанная позиция
	bool m_IsBlocked[4];        // Блокировка направлений: left, right, up, down
	int m_LastUpdateTick;       // Последний тик обновления
	
	// Вспомогательные функции
	bool IsFreezeAt(int x, int y);                    // Проверка фриз тайла в позиции
	vec2 PredictPosition(vec2 pos, vec2 vel, int ticks); // Предсказание позиции
	float CalculateSafeDistance();                    // Расчет безопасной дистанции
	bool IsPathSafe(vec2 from, vec2 to);             // Проверка безопасности пути
	void UpdatePlayerState();                         // Обновление состояния игрока
	void AnalyzeDirection(int direction);             // Анализ направления движения
	
public:
	CFujixBot();
	~CFujixBot();
	
	// Основные функции
	void Init(CGameClient *pGameClient);             // Инициализация
	void OnRender();                                 // Обновление каждый кадр
	void Reset();                                    // Сброс состояния
	
	// Система блокировки движения
	bool ShouldBlockMovement(int direction);        // Проверка блокировки движения (-1=влево, 1=вправо)
	bool ShouldBlockJump();                         // Проверка блокировки прыжка
	bool ShouldBlockHook(vec2 hookTarget);          // Проверка блокировки хука
	bool IsDirectionBlocked(int direction);         // Проверка блокировки направления
	float GetSafeDistance() const { return m_SafeDistance; }
	bool IsActive() const;                          // Активен ли бот
	
	// Константы направлений
	enum EDirection
	{
		DIRECTION_LEFT = 0,
		DIRECTION_RIGHT = 1,
		DIRECTION_UP = 2, 
		DIRECTION_DOWN = 3
	};
};

#endif
