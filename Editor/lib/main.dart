import 'package:editor/game_project/project_browser_dialog.dart';
import 'package:editor/themes/theme_manager.dart';
import 'package:editor/themes/themes.dart';
import 'package:flutter/material.dart';
import 'package:window_manager/window_manager.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await windowManager.ensureInitialized();

  WindowOptions windowOptions = const WindowOptions(
    size: Size(800, 600),
    center: true,
    title: "Lightning Engine",
    minimumSize: Size(800, 600),
    maximumSize: Size(800, 600),
  );

  windowManager.waitUntilReadyToShow(windowOptions, () async {
    await windowManager.show();
    await windowManager.focus();
    await windowManager.setMaximizable(false);
  });

  runApp(const LightningEditor());
}

ThemeManager _themeManager = ThemeManager();

class LightningEditor extends StatelessWidget {
  const LightningEditor({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: "Lightning Editor",
      theme: lightTheme,
      darkTheme: darkTheme,
      themeMode: _themeManager.themeMode,
      home: const ProjectBrowserDialog(),
    );
  }
}
