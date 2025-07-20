#include "fujix_bot.h"

#include <engine/shared/config.h>  // Для g_Config
#include <game/client/gameclient.h>  // Для полного типа CGameClient
#include <game/gamecore.h>  // Для CTuningParams
#include <game/mapitems.h>  // Для констант TILE_FREEZE и т.д.

void CFujixBot::Reset()
{
	m_PlayerPos = vec2(0, 0);
	m_PlayerVel = vec2(0, 0);
	m_PredictedPos = vec2(0, 0);
	m_LastUpdateTick = 0;
	
	// Сброс блокировок
	for(int i = 0; i < 4; i++)
	{
		m_IsBlocked[i] = false;
	}
}

bool CFujixBot::IsActive() const
{
	return g_Config.m_ClFujixGerosBot && m_pGameClient && m_pCollision;
}

void CFujixBot::OnRender()
{
	// Проверяем активность бота
	if(!IsActive())
		return;
		
	// Проверяем есть ли игрок
	if(!m_pGameClient || !m_pGameClient->m_Snap.m_pLocalCharacter)
		return;
	
	// Обновляем состояние игрока
	UpdatePlayerState();
	
	// Анализируем все направления
	for(int dir = 0; dir < 4; dir++)
	{
		AnalyzeDirection(dir);
	}
}

void CFujixBot::UpdatePlayerState()
{
	if(!m_pGameClient || !m_pGameClient->m_Snap.m_pLocalCharacter)
		return;
		
	// Получаем текущую позицию из snap данных  
	vec2 CurrentPos = vec2(m_pGameClient->m_Snap.m_pLocalCharacter->m_X, m_pGameClient->m_Snap.m_pLocalCharacter->m_Y);
	
	// Вычисляем скорость как разность позиций
	if(m_PlayerPos.x != 0 || m_PlayerPos.y != 0)
	{
		m_PlayerVel = CurrentPos - m_PlayerPos;
	}
	
	m_PlayerPos = CurrentPos;
	
	// Обновляем количество тиков для предсказания на основе пинга
	if(m_pGameClient->m_Snap.m_pLocalInfo)
	{
		int ping = m_pGameClient->m_Snap.m_pLocalInfo->m_Latency;
		m_PredictionTicks = maximum(1, ping / 20); // Примерно тик на 20ms пинга
	}
	
	// Предсказываем позицию
	m_PredictedPos = PredictPosition(m_PlayerPos, m_PlayerVel, m_PredictionTicks);
}

vec2 CFujixBot::PredictPosition(vec2 pos, vec2 vel, int ticks)
{
	vec2 predicted = pos;
	vec2 velocity = vel;
	
	// Получаем friction из tuning
	float friction = 0.95f; // Значение по умолчанию (air friction)
	if(m_pGameClient)
	{
		const CTuningParams *pTuning = m_pGameClient->GetTuning(0);
		if(pTuning)
			friction = pTuning->m_AirFriction;
	}
	
	// Симуляция физики на N тиков вперед
	for(int i = 0; i < ticks; i++)
	{
		predicted += velocity;
		velocity *= friction; // Применяем трение каждый тик
	}
	
	return predicted;
}

bool CFujixBot::IsFreezeAt(int x, int y)
{
	if(!m_pCollision)
		return false;
		
	// Проверяем тайл на фриз
	int Tile = m_pCollision->GetTile(x, y);
	int FrontTile = m_pCollision->GetFrontTile(x, y);
	
	// Проверяем все типы фриз тайлов
	return (Tile == TILE_FREEZE || Tile == TILE_DFREEZE || Tile == TILE_LFREEZE ||
	        FrontTile == TILE_FREEZE || FrontTile == TILE_DFREEZE || FrontTile == TILE_LFREEZE);
}

float CFujixBot::CalculateSafeDistance()
{
	// Базовая безопасная дистанция
	float distance = m_SafeDistance;
	
	// Добавляем компенсацию пинга
	if(m_pGameClient->m_Snap.m_pLocalInfo)
	{
		// Получаем пинг игрока (в миллисекундах)
		int ping = m_pGameClient->m_Snap.m_pLocalInfo->m_Latency;
		
		// Конвертируем пинг в дополнительную дистанцию
		// Чем больше пинг, тем больше безопасная дистанция
		float pingFactor = (ping / 100.0f) * m_PingCompensation;
		distance += pingFactor;
	}
	
	return distance;
}

bool CFujixBot::IsPathSafe(vec2 from, vec2 to)
{
	if(!m_pCollision)
		return true;
		
	// Проверяем путь по шагам
	vec2 dir = normalize(to - from);
	float distance = length(to - from);
	float step = 32.0f / 2.0f; // Половина размера тайла для точности
	
	for(float d = 0; d < distance; d += step)
	{
		vec2 checkPos = from + dir * d;
		int tileX = (int)(checkPos.x / 32.0f);
		int tileY = (int)(checkPos.y / 32.0f);
		
		// Если найден фриз тайл - путь небезопасен
		if(IsFreezeAt(tileX, tileY))
			return false;
	}
	
	return true;
}

void CFujixBot::AnalyzeDirection(int direction)
{
	if(!IsActive())
		return;
	
	// Создаем вектор направления движения
	vec2 dirVec(0, 0);
	switch(direction)
	{
		case DIRECTION_LEFT:  dirVec = vec2(-1, 0); break;
		case DIRECTION_RIGHT: dirVec = vec2(1, 0);  break;
		case DIRECTION_UP:    dirVec = vec2(0, -1); break;
		case DIRECTION_DOWN:  dirVec = vec2(0, 1);  break;
	}
	
	// Рассчитываем безопасную дистанцию
	float safeDistance = CalculateSafeDistance() * 32.0f; // 32.0f = размер тайла
	
	// Проверяем точку на безопасной дистанции в этом направлении
	vec2 checkPoint = m_PredictedPos + dirVec * safeDistance;
	
	// Проверяем безопасность пути
	bool pathSafe = IsPathSafe(m_PlayerPos, checkPoint);
	
	// Блокируем направление если путь небезопасен
	m_IsBlocked[direction] = !pathSafe;
}

bool CFujixBot::ShouldBlockMovement(int direction)
{
	if(!IsActive())
		return false;
	
	// Проверяем блокировку движения влево/вправо
	if(direction < 0 && m_IsBlocked[DIRECTION_LEFT])  // Движение влево
		return true;
	if(direction > 0 && m_IsBlocked[DIRECTION_RIGHT]) // Движение вправо  
		return true;
	
	return false;
}

bool CFujixBot::ShouldBlockJump()
{
	if(!IsActive())
		return false;
		
	// Блокируем прыжок если движение вверх заблокировано
	return m_IsBlocked[DIRECTION_UP];
}

bool CFujixBot::ShouldBlockHook(vec2 hookTarget)
{
	if(!IsActive())
		return false;
	
	// Проверяем безопасность пути к цели хука
	return !IsPathSafe(m_PlayerPos, hookTarget);
}

bool CFujixBot::IsDirectionBlocked(int direction)
{
	if(!IsActive())
		return false;
		
	if(direction < 0 || direction >= 4)
		return false;
		
	return m_IsBlocked[direction];
}
