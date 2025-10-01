package com.example.snakedeluxe.ui

import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.material3.rememberTopAppBarState
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.painterResource
import androidx.navigation.NavHostController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.example.snakedeluxe.SnakeViewModel
import com.example.snakedeluxe.ui.screens.GameScreen
import com.example.snakedeluxe.ui.screens.HomeScreen
import com.example.snakedeluxe.ui.screens.SettingsScreen
import com.example.snakedeluxe.ui.screens.SkinsScreen
import com.example.snakedeluxe.ui.theme.SnakeHaptics

private enum class SnakeRoute { Home, Game, Skins, Settings }

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SnakeApp(
    viewModel: SnakeViewModel,
    haptics: SnakeHaptics,
    navController: NavHostController = rememberNavController()
) {
    val state by viewModel.state.collectAsState()
    val appBarState = rememberTopAppBarState()
    val scrollBehavior = TopAppBarDefaults.pinnedScrollBehavior(appBarState)

    Surface(color = MaterialTheme.colorScheme.background) {
        Scaffold(
            topBar = {
                val backStackEntry by navController.currentBackStackEntryFlow.collectAsState(initial = navController.currentBackStackEntry)
                SnakeTopAppBar(
                    currentRoute = backStackEntry?.destination?.route,
                    onBack = { navController.popBackStack() },
                    canNavigateBack = navController.previousBackStackEntry != null,
                    scrollBehavior = scrollBehavior
                )
            },
            modifier = Modifier.navigationBarsPadding()
        ) { innerPadding ->
            NavHost(
                navController = navController,
                startDestination = SnakeRoute.Home.name,
                modifier = Modifier.padding(innerPadding)
            ) {
                composable(SnakeRoute.Home.name) {
                    HomeScreen(
                        state = state,
                        onResume = {
                            viewModel.togglePause()
                            navController.navigate(SnakeRoute.Game.name)
                        },
                        onNewGame = {
                            viewModel.restart()
                            viewModel.togglePause()
                            navController.navigate(SnakeRoute.Game.name)
                        },
                        onSkins = { navController.navigate(SnakeRoute.Skins.name) },
                        onSettings = { navController.navigate(SnakeRoute.Settings.name) }
                    )
                }
                composable(SnakeRoute.Game.name) {
                    GameScreen(
                        state = state,
                        onDirection = {
                            viewModel.changeDirection(it)
                            if (state.settings.vibrationEnabled) haptics.performTick()
                        },
                        onTogglePause = { viewModel.togglePause() },
                        onRestart = {
                            viewModel.restart()
                            viewModel.togglePause()
                            if (state.settings.vibrationEnabled) haptics.performImpact()
                        },
                        onBackToMenu = {
                            if (!state.isPaused) {
                                viewModel.togglePause()
                            }
                            navController.popBackStack()
                        }
                    )
                }
                composable(SnakeRoute.Skins.name) {
                    SkinsScreen(
                        currentSkin = state.skin,
                        skins = com.example.snakedeluxe.model.SnakeSkinPresets.all,
                        onSelect = {
                            viewModel.updateSettings(state.settings.copy(selectedSkinId = it.id))
                            navController.popBackStack()
                        }
                    )
                }
                composable(SnakeRoute.Settings.name) {
                    SettingsScreen(
                        settings = state.settings,
                        onSettingsChanged = { viewModel.updateSettings(it) }
                    )
                }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun SnakeTopAppBar(
    currentRoute: String?,
    canNavigateBack: Boolean,
    onBack: () -> Unit,
    scrollBehavior: TopAppBarDefaults.ExitUntilCollapsedScrollBehavior
) {
    val title = when (currentRoute) {
        SnakeRoute.Game.name -> "Аркада"
        SnakeRoute.Skins.name -> "Скины"
        SnakeRoute.Settings.name -> "Настройки"
        else -> "Snake Deluxe"
    }
    TopAppBar(
        title = { Text(text = title) },
        navigationIcon = {
            if (canNavigateBack) {
                IconButton(onClick = onBack) {
                    Icon(
                        painter = painterResource(id = android.R.drawable.ic_media_previous),
                        contentDescription = null
                    )
                }
            }
        },
        scrollBehavior = scrollBehavior
    )
}
