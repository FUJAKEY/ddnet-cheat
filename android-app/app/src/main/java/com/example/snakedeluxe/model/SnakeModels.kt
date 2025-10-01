package com.example.snakedeluxe.model

import androidx.compose.ui.graphics.Color
import kotlin.math.absoluteValue

enum class Direction(val dx: Int, val dy: Int) {
    Up(0, -1),
    Down(0, 1),
    Left(-1, 0),
    Right(1, 0);

    fun isOpposite(other: Direction): Boolean = dx + other.dx == 0 && dy + other.dy == 0
}

data class GridPosition(val x: Int, val y: Int) {
    operator fun plus(direction: Direction): GridPosition = GridPosition(x + direction.dx, y + direction.dy)

    fun wrap(width: Int, height: Int): GridPosition = GridPosition((x + width) % width, (y + height) % height)

    fun distance(other: GridPosition): Int = (x - other.x).absoluteValue + (y - other.y).absoluteValue
}

data class SnakeSkin(
    val id: String,
    val displayName: String,
    val snakeColor: Color,
    val foodColor: Color,
    val boardColor: Color,
    val accentColor: Color
)

data class GameSettings(
    val speedMultiplier: Float = 1.0f,
    val width: Int = 18,
    val height: Int = 28,
    val soundEnabled: Boolean = true,
    val vibrationEnabled: Boolean = true,
    val selectedSkinId: String = SnakeSkinPresets.Classic.id
)

data class GameState(
    val snake: List<GridPosition>,
    val direction: Direction,
    val food: GridPosition,
    val pendingGrowth: Int,
    val score: Int,
    val bestScore: Int,
    val isPaused: Boolean,
    val isGameOver: Boolean,
    val settings: GameSettings,
    val skin: SnakeSkin
)

object SnakeSkinPresets {
    val Classic = SnakeSkin(
        id = "classic",
        displayName = "Классика",
        snakeColor = Color(0xFF4CAF50),
        foodColor = Color(0xFFFFC107),
        boardColor = Color(0xFF101820),
        accentColor = Color(0xFF00BCD4)
    )

    val Neon = SnakeSkin(
        id = "neon",
        displayName = "Неон",
        snakeColor = Color(0xFF7C4DFF),
        foodColor = Color(0xFFE91E63),
        boardColor = Color(0xFF05070A),
        accentColor = Color(0xFF40C4FF)
    )

    val Sunset = SnakeSkin(
        id = "sunset",
        displayName = "Закат",
        snakeColor = Color(0xFFFF7043),
        foodColor = Color(0xFFFFEE58),
        boardColor = Color(0xFF2D132C),
        accentColor = Color(0xFFFF9E80)
    )

    val Ice = SnakeSkin(
        id = "ice",
        displayName = "Лёд",
        snakeColor = Color(0xFF26C6DA),
        foodColor = Color(0xFF00E5FF),
        boardColor = Color(0xFF001F3F),
        accentColor = Color(0xFF81D4FA)
    )

    val all = listOf(Classic, Neon, Sunset, Ice)

    fun findById(id: String): SnakeSkin = all.find { it.id == id } ?: Classic
}
