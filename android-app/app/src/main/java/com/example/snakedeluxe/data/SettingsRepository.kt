package com.example.snakedeluxe.data

import android.content.Context
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.booleanPreferencesKey
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.floatPreferencesKey
import androidx.datastore.preferences.core.intPreferencesKey
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import com.example.snakedeluxe.model.GameSettings
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.catch
import kotlinx.coroutines.flow.map
import java.io.IOException

private val Context.dataStore by preferencesDataStore("snake_settings")

class SettingsRepository(private val context: Context) {

    private object Keys {
        val Speed = floatPreferencesKey("speed")
        val Width = intPreferencesKey("width")
        val Height = intPreferencesKey("height")
        val Sound = booleanPreferencesKey("sound")
        val Vibration = booleanPreferencesKey("vibration")
        val Skin = stringPreferencesKey("skin")
        val BestScore = intPreferencesKey("best_score")
    }

    val settings: Flow<GameSettings> = context.dataStore.data
        .catch { exception ->
            if (exception is IOException) emit(emptyPreferences()) else throw exception
        }
        .map { preferences -> preferences.toSettings() }

    val bestScore: Flow<Int> = context.dataStore.data
        .catch { exception ->
            if (exception is IOException) emit(emptyPreferences()) else throw exception
        }
        .map { preferences -> preferences[Keys.BestScore] ?: 0 }

    suspend fun updateSettings(settings: GameSettings) {
        context.dataStore.edit { preferences ->
            preferences[Keys.Speed] = settings.speedMultiplier
            preferences[Keys.Width] = settings.width
            preferences[Keys.Height] = settings.height
            preferences[Keys.Sound] = settings.soundEnabled
            preferences[Keys.Vibration] = settings.vibrationEnabled
            preferences[Keys.Skin] = settings.selectedSkinId
        }
    }

    suspend fun updateBestScore(score: Int) {
        context.dataStore.edit { preferences ->
            if ((preferences[Keys.BestScore] ?: 0) < score) {
                preferences[Keys.BestScore] = score
            }
        }
    }

    private fun Preferences.toSettings(): GameSettings = GameSettings(
        speedMultiplier = this[Keys.Speed] ?: 1.0f,
        width = this[Keys.Width] ?: 18,
        height = this[Keys.Height] ?: 28,
        soundEnabled = this[Keys.Sound] ?: true,
        vibrationEnabled = this[Keys.Vibration] ?: true,
        selectedSkinId = this[Keys.Skin] ?: GameSettings().selectedSkinId
    )
}

private fun emptyPreferences(): Preferences = androidx.datastore.preferences.core.emptyPreferences()
