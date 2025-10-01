package com.example.snakedeluxe.ui.theme

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.graphics.Color

private val DarkColorScheme = darkColorScheme(
    primary = Color(0xFF4CAF50),
    secondary = Color(0xFF00BCD4),
    background = Color(0xFF101820),
    surface = Color(0xFF182430),
    onSurface = Color(0xFFE8F1F2)
)

private val LightColorScheme = lightColorScheme(
    primary = Color(0xFF4CAF50),
    secondary = Color(0xFF00BCD4),
    background = Color(0xFFF7F9FC),
    surface = Color(0xFFFFFFFF),
    onSurface = Color(0xFF20252B)
)

@Composable
fun SnakeDeluxeTheme(
    darkTheme: Boolean = isSystemInDarkTheme(),
    content: @Composable () -> Unit
) {
    val colorScheme = remember(darkTheme) { if (darkTheme) DarkColorScheme else LightColorScheme }

    MaterialTheme(
        colorScheme = colorScheme,
        typography = Typography,
        shapes = Shapes,
        content = content
    )
}
