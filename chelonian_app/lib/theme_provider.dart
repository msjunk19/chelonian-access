import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';


class ThemeProvider extends ChangeNotifier {
  late SharedPreferences _prefs;
  late bool _isDarkMode;

  bool get isDarkMode => _isDarkMode;

  ThemeProvider() {
    _isDarkMode = false;
    _initPreferences();
  }

  Future<void> _initPreferences() async {
    _prefs = await SharedPreferences.getInstance();
    _isDarkMode = _prefs.getBool('darkMode') ?? false;
    notifyListeners();
  }

  Future<void> toggleTheme() async {
    _isDarkMode = !_isDarkMode;
    await _prefs.setBool('darkMode', _isDarkMode);
    notifyListeners();
  }

  ThemeData get lightTheme {
    return ThemeData(
      useMaterial3: true,
      brightness: Brightness.light,
      primaryColor: const Color(0xFF3B82F6),
      scaffoldBackgroundColor: const Color(0xFFF3F4F6),
      cardColor: Colors.white,
      colorScheme: ColorScheme.light(
        primary: const Color(0xFF3B82F6),
        secondary: const Color(0xFF22C55E),
        error: const Color(0xFFEF4444),
        surface: const Color(0xFFF3F4F6),
        onSurface: const Color(0xFF1F2937),
      ),
      appBarTheme: AppBarTheme(
        backgroundColor: Colors.white,
        foregroundColor: const Color(0xFF1F2937),
        elevation: 0,
        iconTheme: const IconThemeData(color: Color(0xFF1F2937)),
      ),
      textTheme: const TextTheme(
        bodyLarge: TextStyle(color: Color(0xFF1F2937)),
        bodyMedium: TextStyle(color: Color(0xFF1F2937)),
        titleLarge: TextStyle(color: Color(0xFF1F2937), fontWeight: FontWeight.bold),
      ),
      inputDecorationTheme: InputDecorationTheme(
        filled: true,
        fillColor: const Color(0xFFF9FAFB),
        border: OutlineInputBorder(
          borderRadius: BorderRadius.circular(12),
          borderSide: const BorderSide(color: Color(0xFFE5E7EB)),
        ),
        enabledBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(12),
          borderSide: const BorderSide(color: Color(0xFFE5E7EB)),
        ),
        focusedBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(12),
          borderSide: const BorderSide(color: Color(0xFF3B82F6), width: 2),
        ),
        contentPadding: EdgeInsets.symmetric(horizontal: 12, vertical: 8),

      ),
      elevatedButtonTheme: ElevatedButtonThemeData(
        style: ElevatedButton.styleFrom(
          backgroundColor: const Color(0xFF3B82F6),
          foregroundColor: Colors.white,
          padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(12),
          ),
        ),
      ),
    );
  }

  ThemeData get darkTheme {
    return ThemeData(
      useMaterial3: true,
      brightness: Brightness.dark,
      primaryColor: const Color(0xFF3B82F6),
      scaffoldBackgroundColor: const Color(0xFF111827),
      cardColor: const Color(0xFF1F2937),
      colorScheme: ColorScheme.dark(
        primary: const Color(0xFF3B82F6),
        secondary: const Color(0xFF22C55E),
        error: const Color(0xFFEF4444),
        surface: const Color(0xFF111827),
        onSurface: const Color(0xFFF9FAFB),
      ),
      appBarTheme: AppBarTheme(
        backgroundColor: const Color(0xFF1F2937),
        foregroundColor: const Color(0xFFF9FAFB),
        elevation: 0,
        iconTheme: const IconThemeData(color: Color(0xFFF9FAFB)),
      ),
      textTheme: const TextTheme(
        bodyLarge: TextStyle(color: Color(0xFFF9FAFB)),
        bodyMedium: TextStyle(color: Color(0xFFF9FAFB)),
        titleLarge: TextStyle(color: Color(0xFFF9FAFB), fontWeight: FontWeight.bold),
      ),
      inputDecorationTheme: InputDecorationTheme(
        filled: true,
        fillColor: const Color(0xFF374151),
        border: OutlineInputBorder(
          borderRadius: BorderRadius.circular(12),
          borderSide: const BorderSide(color: Color(0xFF374151)),
        ),

        enabledBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(12),
          borderSide: const BorderSide(color: Color(0xFF374151)),
        ),
        focusedBorder: OutlineInputBorder(
          borderRadius: BorderRadius.circular(12),
          borderSide: const BorderSide(color: Color(0xFF3B82F6), width: 2),
        ),
        contentPadding: EdgeInsets.symmetric(horizontal: 12, vertical: 8),

      ),
      elevatedButtonTheme: ElevatedButtonThemeData(
        style: ElevatedButton.styleFrom(
          backgroundColor: const Color(0xFF3B82F6),
          foregroundColor: Colors.white,
          padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(12),
          ),
        ),
      ),
    );
  }
}