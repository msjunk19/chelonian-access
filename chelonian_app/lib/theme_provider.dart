import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';


class ThemeProvider with ChangeNotifier {
  bool _isDark = false;
  bool get isDark => _isDark;

  Future<void> load(BuildContext context) async {
    final prefs = await SharedPreferences.getInstance();
    _isDark = prefs.getBool('dark_mode') ?? false;
    notifyListeners();
  }

  Future<void> toggle(BuildContext context) async {
    final prefs = await SharedPreferences.getInstance();
    _isDark = !(prefs.getBool('dark_mode') ?? false);
    await prefs.setBool('dark_mode', _isDark);
    notifyListeners();

    // Apply theme across the app
    final theme = _isDark ? ThemeData.dark() : ThemeData.light();
    if (context.mounted) {
      Navigator.of(context).pushReplacement(MaterialPageRoute(
        builder: (context) => MaterialApp(theme: theme, home: Navigator.of(context).widget),
      ));
    }
  }
}