package com.example.snakedeluxe.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import com.example.snakedeluxe.model.SnakeSkin

@Composable
fun SkinsScreen(
    currentSkin: SnakeSkin,
    skins: List<SnakeSkin>,
    onSelect: (SnakeSkin) -> Unit
) {
    LazyColumn(
        modifier = Modifier.padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp),
        contentPadding = PaddingValues(bottom = 32.dp)
    ) {
        items(skins) { skin ->
            SkinCard(
                skin = skin,
                selected = skin.id == currentSkin.id,
                onSelect = onSelect
            )
        }
    }
}

@Composable
private fun SkinCard(skin: SnakeSkin, selected: Boolean, onSelect: (SnakeSkin) -> Unit) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .clickable { onSelect(skin) },
        shape = RoundedCornerShape(24.dp),
        colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surface.copy(alpha = 0.85f))
    ) {
        Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
            Box(
                modifier = Modifier
                    .background(
                        brush = Brush.verticalGradient(
                            listOf(skin.boardColor.copy(alpha = 0.9f), skin.accentColor.copy(alpha = 0.6f))
                        ),
                        shape = RoundedCornerShape(topStart = 24.dp, topEnd = 24.dp)
                    )
                    .padding(24.dp)
            ) {
                Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
                    Text(text = skin.displayName, style = MaterialTheme.typography.titleLarge, color = Color.White)
                    Text(text = "Цвет змеи", color = skin.snakeColor)
                    Text(text = "Акценты", color = skin.accentColor)
                }
            }
            if (selected) {
                Text(
                    text = "Выбрано",
                    modifier = Modifier
                        .align(Alignment.End)
                        .padding(16.dp),
                    color = MaterialTheme.colorScheme.primary,
                    style = MaterialTheme.typography.labelLarge
                )
            }
        }
    }
}
