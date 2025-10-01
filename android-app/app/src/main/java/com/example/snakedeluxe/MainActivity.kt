package com.example.snakedeluxe

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.viewModels
import androidx.core.view.WindowCompat
import com.example.snakedeluxe.ui.theme.SnakeDeluxeTheme
import com.example.snakedeluxe.ui.theme.rememberSnakeHaptics
import com.example.snakedeluxe.ui.SnakeApp

class MainActivity : ComponentActivity() {
    private val viewModel: SnakeViewModel by viewModels { SnakeViewModel.Factory(applicationContext) }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        WindowCompat.setDecorFitsSystemWindows(window, false)

        setContent {
            val haptics = rememberSnakeHaptics()
            SnakeDeluxeTheme {
                SnakeApp(
                    viewModel = viewModel,
                    haptics = haptics
                )
            }
        }
    }
}
