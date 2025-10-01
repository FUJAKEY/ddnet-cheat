package com.example.snakedeluxe.ui.theme

import android.os.VibrationEffect
import android.os.Vibrator
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.platform.LocalContext
import androidx.core.content.getSystemService

class SnakeHaptics(private val vibrator: Vibrator?) {
    fun performTick() {
        vibrator?.vibrate(VibrationEffect.createOneShot(16, VibrationEffect.EFFECT_TICK))
    }

    fun performImpact() {
        vibrator?.vibrate(VibrationEffect.createOneShot(32, VibrationEffect.EFFECT_HEAVY_CLICK))
    }
}

@Composable
fun rememberSnakeHaptics(): SnakeHaptics {
    val context = LocalContext.current
    val vibrator = remember(context) { context.getSystemService<Vibrator>() }
    return remember(vibrator) { SnakeHaptics(vibrator) }
}
