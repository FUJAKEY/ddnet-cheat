#include "fujix_bot.h"

#include <game/mapitems.h>  // Для констант TILE_FREEZE и т.д.

CFujixBot::~CFujixBot()
{
}

void CFujixBot::Init(CGameClient *pGameClient)
{
	m_pGameClient = pGameClient;
	if(m_pGameClient)
	{
		m_pCollision = m_pGameClient->Collision();
	}
	Reset();
}

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
	if(!m_pGameClient->m_LocalCharacterPos.x && !m_pGameClient->m_LocalCharacterPos.y)
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
	// Получаем текущую позицию игрока
	m_PlayerPos = vec2(m_pGameClient->m_LocalCharacterPos.x, m_pGameClient->m_LocalCharacterPos.y);
	
	// Получаем скорость игрока
	if(m_pGameClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_Direction)
	{
		int dir = m_pGameClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_Direction;
		m_PlayerVel.x = dir * PLAYER_SPEED;
	}
	else
	{
		m_PlayerVel.x *= FRICTION; // Применяем трение
	}
	
	// Прыжок влияет на вертикальную скорость
	if(m_pGameClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_Jump)
	{
		m_PlayerVel.y = -PLAYER_SPEED; // Отрицательное значение = вверх
	}
	else
	{
		m_PlayerVel.y *= FRICTION;
	}
	
	// Предсказываем позицию
	m_PredictedPos = PredictPosition(m_PlayerPos, m_PlayerVel, m_PredictionTicks);
}

vec2 CFujixBot::PredictPosition(vec2 pos, vec2 vel, int ticks)
{
	vec2 predicted = pos;
	vec2 velocity = vel;
	
	// Симуляция физики на N тиков вперед
	for(int i = 0; i < ticks; i++)
	{
		predicted += velocity;
		velocity *= FRICTION; // Применяем трение каждый тик
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
	// Проверяем путь от from до to по линии
	vec2 direction = normalize(to - from);
	float distance = length(to - from);
	
	// Шагаем по пути с шагом в половину тайла
	float step = TILE_SIZE / 2.0f;
	for(float d = 0; d < distance; d += step)
	{
		vec2 checkPos = from + direction * d;
		
		// Конвертируем в координаты тайлов
		int tileX = (int)(checkPos.x / TILE_SIZE);
		int tileY = (int)(checkPos.y / TILE_SIZE);
		
		// Проверяем фриз тайл
		if(IsFreezeAt(tileX, tileY))
		{
			return false; // Найден фриз тайл на пути
		}
	}
	
	return true; // Путь безопасен
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
	float safeDistance = CalculateSafeDistance() * TILE_SIZE;
	
	// Проверяем точку на безопасной дистанции в этом направлении
	vec2 checkPoint = m_PlayerPos + dirVec * safeDistance;
	
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
