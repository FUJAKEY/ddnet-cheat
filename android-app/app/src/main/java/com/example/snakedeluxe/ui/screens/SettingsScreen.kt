package com.example.snakedeluxe.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Slider
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.example.snakedeluxe.R
import com.example.snakedeluxe.model.GameSettings

@Composable
fun SettingsScreen(
    settings: GameSettings,
    onSettingsChanged: (GameSettings) -> Unit
) {
    val localSettings = remember(settings) { mutableStateOf(settings) }

    Card(
        modifier = Modifier
            .padding(16.dp)
            .fillMaxWidth(),
        colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surface.copy(alpha = 0.8f))
    ) {
        Column(modifier = Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(16.dp)) {
            Text(text = stringResource(id = R.string.settings), style = MaterialTheme.typography.titleLarge)
            SliderWithLabel(
                label = stringResource(id = R.string.speed, localSettings.value.speedMultiplier),
                value = localSettings.value.speedMultiplier,
                onValueChange = { value -> localSettings.value = localSettings.value.copy(speedMultiplier = value) },
                valueRange = 0.5f..2.5f
            )
            SliderWithLabel(
                label = stringResource(id = R.string.grid_size, localSettings.value.width, localSettings.value.height),
                value = localSettings.value.width.toFloat(),
                onValueChange = { value ->
                    val size = value.toInt().coerceIn(12, 32)
                    localSettings.value = localSettings.value.copy(width = size, height = (size * 1.5f).toInt())
                },
                valueRange = 12f..32f
            )
            SwitchRow(
                label = stringResource(id = R.string.sound),
                checked = localSettings.value.soundEnabled,
                onCheckedChange = { isChecked -> localSettings.value = localSettings.value.copy(soundEnabled = isChecked) }
            )
            SwitchRow(
                label = stringResource(id = R.string.vibration),
                checked = localSettings.value.vibrationEnabled,
                onCheckedChange = { isChecked -> localSettings.value = localSettings.value.copy(vibrationEnabled = isChecked) }
            )
            Button(onClick = { onSettingsChanged(localSettings.value) }) {
                Text(text = stringResource(id = R.string.apply))
            }
        }
    }
}

@Composable
private fun SliderWithLabel(
    label: String,
    value: Float,
    onValueChange: (Float) -> Unit,
    valueRange: ClosedFloatingPointRange<Float>
) {
    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
        Text(text = label, style = MaterialTheme.typography.titleMedium)
        Slider(value = value, onValueChange = onValueChange, valueRange = valueRange)
    }
}

@Composable
private fun SwitchRow(label: String, checked: Boolean, onCheckedChange: (Boolean) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(text = label, style = MaterialTheme.typography.bodyLarge)
        Switch(checked = checked, onCheckedChange = onCheckedChange)
    }
}
