package com.example.snakedeluxe

import android.content.Context
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.example.snakedeluxe.data.SettingsRepository
import com.example.snakedeluxe.model.Direction
import com.example.snakedeluxe.model.GameSettings
import com.example.snakedeluxe.model.GameState
import com.example.snakedeluxe.model.GridPosition
import com.example.snakedeluxe.model.SnakeSkinPresets
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch
import kotlin.random.Random

class SnakeViewModel(private val repository: SettingsRepository) : ViewModel() {

    private val random = Random(System.currentTimeMillis())
    private val gameState = MutableStateFlow(createInitialState())
    private var ticker: Job? = null

    val state: StateFlow<GameState> = combine(
        repository.settings,
        repository.bestScore,
        gameState
    ) { settings, bestScore, game ->
        game.copy(
            settings = settings,
            skin = SnakeSkinPresets.findById(settings.selectedSkinId),
            bestScore = bestScore
        )
    }.stateIn(viewModelScope, SharingStarted.Eagerly, gameState.value)

    init {
        startTicker()
        viewModelScope.launch {
            repository.settings.collect { settings ->
                gameState.value = gameState.value.copy(
                    settings = settings,
                    skin = SnakeSkinPresets.findById(settings.selectedSkinId)
                )
            }
        }
        viewModelScope.launch {
            repository.bestScore.collect { best ->
                gameState.value = gameState.value.copy(bestScore = best)
            }
        }
    }

    private fun startTicker() {
        ticker?.cancel()
        ticker = viewModelScope.launch {
            while (true) {
                val speed = (200L / gameState.value.settings.speedMultiplier).coerceAtLeast(60L)
                delay(speed)
                step()
            }
        }
    }

    fun togglePause() {
        gameState.value = gameState.value.copy(isPaused = !gameState.value.isPaused)
    }

    fun restart() {
        gameState.value = createInitialState().copy(bestScore = gameState.value.bestScore)
    }

    fun changeDirection(direction: Direction) {
        val current = gameState.value
        if (current.isPaused || current.isGameOver) return
        if (!direction.isOpposite(current.direction)) {
            gameState.value = current.copy(direction = direction)
        }
    }

    private fun step() {
        val current = gameState.value
        if (current.isPaused || current.isGameOver) return

        val nextHead = (current.snake.first() + current.direction).wrap(current.settings.width, current.settings.height)
        if (current.snake.contains(nextHead)) {
            viewModelScope.launch { repository.updateBestScore(current.score) }
            gameState.value = current.copy(isGameOver = true, isPaused = true)
            return
        }

        val ateFood = nextHead == current.food
        val growthBudget = current.pendingGrowth + if (ateFood) 2 else 0
        val grownSnake = listOf(nextHead) + current.snake
        val trimmedSnake = if (growthBudget > 0) grownSnake else grownSnake.dropLast(1)
        val remainingGrowth = (growthBudget - 1).coerceAtLeast(0)
        val nextFood = if (ateFood) spawnFood(trimmedSnake, current.settings.width, current.settings.height) else current.food
        val nextScore = current.score + if (ateFood) 10 else 0

        gameState.value = current.copy(
            snake = trimmedSnake,
            food = nextFood,
            pendingGrowth = remainingGrowth,
            score = nextScore
        )
        if (nextScore > current.bestScore) {
            viewModelScope.launch { repository.updateBestScore(nextScore) }
        }
    }

    fun updateSettings(settings: GameSettings) {
        viewModelScope.launch {
            repository.updateSettings(settings)
            gameState.value = if (settings.width != gameState.value.settings.width || settings.height != gameState.value.settings.height) {
                createInitialState(settings).copy(bestScore = gameState.value.bestScore)
            } else {
                gameState.value.copy(settings = settings, skin = SnakeSkinPresets.findById(settings.selectedSkinId))
            }
            startTicker()
        }
    }

    private fun spawnFood(snake: List<GridPosition>, width: Int, height: Int): GridPosition {
        val occupied = snake.toSet()
        var candidate: GridPosition
        do {
            candidate = GridPosition(random.nextInt(width), random.nextInt(height))
        } while (candidate in occupied)
        return candidate
    }

    companion object {
        private fun createInitialState(settings: GameSettings = GameSettings()): GameState {
            val initialSnake = listOf(
                GridPosition(settings.width / 2, settings.height / 2),
                GridPosition(settings.width / 2 - 1, settings.height / 2),
                GridPosition(settings.width / 2 - 2, settings.height / 2)
            )
            return GameState(
                snake = initialSnake,
                direction = Direction.Right,
                food = GridPosition(settings.width / 3, settings.height / 3),
                pendingGrowth = 0,
                score = 0,
                bestScore = 0,
                isPaused = true,
                isGameOver = false,
                settings = settings,
                skin = SnakeSkinPresets.findById(settings.selectedSkinId)
            )
        }
    }

    class Factory(private val context: Context) : ViewModelProvider.Factory {
        override fun <T : ViewModel> create(modelClass: Class<T>): T {
            val repository = SettingsRepository(context)
            return SnakeViewModel(repository) as T
        }
    }
}
