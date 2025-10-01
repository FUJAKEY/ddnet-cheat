package com.example.snakedeluxe.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Pause
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.FilledTonalIconButton
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.example.snakedeluxe.R
import com.example.snakedeluxe.model.Direction
import com.example.snakedeluxe.model.GameState
import com.example.snakedeluxe.ui.components.SnakeBoard

@Composable
fun GameScreen(
    state: GameState,
    onDirection: (Direction) -> Unit,
    onTogglePause: () -> Unit,
    onRestart: () -> Unit,
    onBackToMenu: () -> Unit
) {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        Card(colors = CardDefaults.cardColors(containerColor = state.skin.boardColor.copy(alpha = 0.6f))) {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(16.dp)
            ) {
                Text(
                    text = "Счёт: ${state.score}",
                    style = MaterialTheme.typography.titleLarge,
                    color = state.skin.accentColor
                )
                Text(
                    text = "Рекорд: ${state.bestScore}",
                    style = MaterialTheme.typography.bodyLarge,
                    color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.7f)
                )
            }
        }
        Card(
            modifier = Modifier.weight(1f),
            colors = CardDefaults.cardColors(containerColor = Color.Transparent)
        ) {
            Box(modifier = Modifier.fillMaxSize()) {
                SnakeBoard(
                    state = state,
                    onSwipe = onDirection,
                    modifier = Modifier.fillMaxSize()
                )
                if (state.isPaused || state.isGameOver) {
                    PauseOverlay(
                        isGameOver = state.isGameOver,
                        score = state.score,
                        bestScore = state.bestScore,
                        onTogglePause = onTogglePause,
                        onRestart = onRestart
                    )
                }
            }
        }
        GameControls(
            isPaused = state.isPaused,
            onDirection = onDirection,
            onTogglePause = onTogglePause,
            onRestart = onRestart,
            onBackToMenu = onBackToMenu,
            accent = state.skin.accentColor
        )
    }
}

@Composable
private fun PauseOverlay(
    isGameOver: Boolean,
    score: Int,
    bestScore: Int,
    onTogglePause: () -> Unit,
    onRestart: () -> Unit
) {
    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(Color.Black.copy(alpha = 0.45f)),
        contentAlignment = Alignment.Center
    ) {
        Card(
            colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surface.copy(alpha = 0.95f))
        ) {
            Column(
                modifier = Modifier.padding(24.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp),
                horizontalAlignment = Alignment.CenterHorizontally
            ) {
                Text(
                    text = if (isGameOver) stringResource(id = R.string.game_over) else stringResource(id = R.string.pause),
                    style = MaterialTheme.typography.headlineMedium
                )
                Text(text = stringResource(id = R.string.score, score), style = MaterialTheme.typography.titleLarge)
                Text(text = stringResource(id = R.string.high_score, bestScore), style = MaterialTheme.typography.bodyLarge)
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(12.dp, Alignment.CenterHorizontally)
                ) {
                    Button(onClick = onRestart) { Text(stringResource(id = R.string.restart)) }
                    if (!isGameOver) {
                        Button(
                            onClick = onTogglePause,
                            colors = ButtonDefaults.buttonColors(containerColor = MaterialTheme.colorScheme.primary)
                        ) {
                            Text(stringResource(id = R.string.resume))
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun GameControls(
    isPaused: Boolean,
    onDirection: (Direction) -> Unit,
    onTogglePause: () -> Unit,
    onRestart: () -> Unit,
    onBackToMenu: () -> Unit,
    accent: Color
) {
    Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Button(onClick = onBackToMenu) { Text(text = stringResource(id = R.string.home)) }
            IconButton(onClick = onTogglePause) {
                Icon(
                    imageVector = if (isPaused) Icons.Default.PlayArrow else Icons.Default.Pause,
                    contentDescription = null
                )
            }
            Button(onClick = onRestart, colors = ButtonDefaults.buttonColors(containerColor = accent)) {
                Text(text = stringResource(id = R.string.restart))
            }
        }
        Spacer(modifier = Modifier.height(8.dp))
        DirectionalPad(onDirection = onDirection, accent = accent)
    }
}

@Composable
private fun DirectionalPad(onDirection: (Direction) -> Unit, accent: Color) {
    Column(horizontalAlignment = Alignment.CenterHorizontally) {
        FilledTonalIconButton(
            onClick = { onDirection(Direction.Up) },
            shape = CircleShape,
            colors = ButtonDefaults.filledTonalIconButtonColors(containerColor = accent.copy(alpha = 0.2f))
        ) {
            Text("↑", style = MaterialTheme.typography.titleLarge)
        }
        Row(horizontalArrangement = Arrangement.spacedBy(32.dp), verticalAlignment = Alignment.CenterVertically) {
            FilledTonalIconButton(
                onClick = { onDirection(Direction.Left) },
                shape = CircleShape,
                colors = ButtonDefaults.filledTonalIconButtonColors(containerColor = accent.copy(alpha = 0.2f))
            ) { Text("←", style = MaterialTheme.typography.titleLarge) }
            FilledTonalIconButton(
                onClick = { onDirection(Direction.Right) },
                shape = CircleShape,
                colors = ButtonDefaults.filledTonalIconButtonColors(containerColor = accent.copy(alpha = 0.2f))
            ) { Text("→", style = MaterialTheme.typography.titleLarge) }
        }
        FilledTonalIconButton(
            onClick = { onDirection(Direction.Down) },
            shape = CircleShape,
            colors = ButtonDefaults.filledTonalIconButtonColors(containerColor = accent.copy(alpha = 0.2f))
        ) {
            Text("↓", style = MaterialTheme.typography.titleLarge)
        }
    }
}
