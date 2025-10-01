package com.example.snakedeluxe.ui.components

import androidx.compose.animation.core.Animatable
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.consumeAllChanges
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.unit.dp
import com.example.snakedeluxe.model.Direction
import com.example.snakedeluxe.model.GameState
import kotlin.math.abs
import kotlin.math.max
import kotlin.math.min

@Composable
fun SnakeBoard(
    state: GameState,
    onSwipe: (Direction) -> Unit,
    modifier: Modifier = Modifier
) {
    val glow = remember { Animatable(0f) }

    LaunchedEffect(state.score) {
        glow.animateTo(1f, tween(240))
        glow.animateTo(0f, tween(480))
    }

    Box(
        modifier = modifier
            .background(state.skin.boardColor.copy(alpha = 0.9f), shape = MaterialTheme.shapes.large)
            .padding(12.dp)
            .pointerInput(Unit) {
                detectDragGestures { change, dragAmount ->
                    change.consumeAllChanges()
                    val (x, y) = dragAmount
                    if (abs(x) > abs(y)) {
                        if (x > 0) onSwipe(Direction.Right) else onSwipe(Direction.Left)
                    } else {
                        if (y > 0) onSwipe(Direction.Down) else onSwipe(Direction.Up)
                    }
                }
            }
    ) {
        Canvas(modifier = Modifier.fillMaxSize()) {
            val cellSize = min(size.width / state.settings.width, size.height / state.settings.height)
            drawBoard(state, cellSize)
            drawSnake(state, cellSize, glow.value)
            drawFood(state, cellSize)
        }
    }
}

private fun androidx.compose.ui.graphics.drawscope.DrawScope.drawBoard(state: GameState, cellSize: Float) {
    val gridColor = state.skin.boardColor.copy(alpha = 0.35f)
    for (x in 0 until state.settings.width) {
        for (y in 0 until state.settings.height) {
            drawRoundRect(
                color = gridColor,
                topLeft = Offset(x * cellSize, y * cellSize),
                size = Size(cellSize - 3f, cellSize - 3f),
                cornerRadius = CornerRadius(8f, 8f)
            )
        }
    }
}

private fun androidx.compose.ui.graphics.drawscope.DrawScope.drawSnake(state: GameState, cellSize: Float, glow: Float) {
    val snakeColor = Color.lerp(state.skin.snakeColor, state.skin.accentColor, glow)
    state.snake.forEachIndexed { index, segment ->
        val factor = 1f - index / (state.snake.size.toFloat() * 1.1f)
        val radius = max(6f, 16f * factor)
        drawRoundRect(
            color = snakeColor.copy(alpha = 0.95f - index * 0.01f),
            topLeft = Offset(segment.x * cellSize, segment.y * cellSize),
            size = Size(cellSize - 2f, cellSize - 2f),
            cornerRadius = CornerRadius(radius, radius)
        )
    }
}

private fun androidx.compose.ui.graphics.drawscope.DrawScope.drawFood(state: GameState, cellSize: Float) {
    val size = cellSize * 0.85f
    val topLeft = Offset(
        state.food.x * cellSize + (cellSize - size) / 2,
        state.food.y * cellSize + (cellSize - size) / 2
    )
    drawRoundRect(
        color = state.skin.foodColor,
        topLeft = topLeft,
        size = Size(size, size),
        cornerRadius = CornerRadius(size / 2, size / 2)
    )
}
