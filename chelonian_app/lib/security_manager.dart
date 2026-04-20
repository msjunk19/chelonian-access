import 'dart:convert';
import 'dart:math';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:flutter/foundation.dart';

// ─────────────────────────────────────────────
// Security Manager for Flutter App
// ─────────────────────────────────────────────

class SecurityManager {
  static const int NONCE_EXPIRY_MS = 30000;  // 30 seconds (must match device)
  
  final SharedPreferences prefs;
  late final NonceManager _nonceManager;
  
  SecurityManager(this.prefs) {
    _nonceManager = NonceManager();
  }

  // ── Nonce Management ──────────────────────────────────────────────

  /// Generate a fresh nonce for command execution
  /// Must call /api/nonce endpoint to get nonce from device first
  String generateLocalNonce() {
    return _nonceManager.generateNonce();
  }

  /// Validate that returned nonce is recent and unused
  bool validateNonceResponse(String nonce) {
    if (nonce.isEmpty || nonce.length != 32) {
      debugPrint('Invalid nonce format: $nonce');
      return false;
    }
    return _nonceManager.isValidNonce(nonce);
  }

  // ── Token Encryption ─────────────────────────────────────────────

  /// Encrypt token for storage (AES-256-GCM simulation)
  /// Note: Flutter crypto is simplified; use native implementation for production
  String encryptToken(String token) {
    // For now, just base64 encode
    // In production, use pointycastle or flutter_sodium for AES-256-GCM
    return base64Encode(utf8.encode(token));
  }

  /// Decrypt token from storage
  String? decryptToken(String encryptedToken) {
    try {
      return utf8.decode(base64Decode(encryptedToken));
    } catch (e) {
      debugPrint('Token decryption failed: $e');
      return null;
    }
  }

  // ── Timestamp Validation ──────────────────────────────────────────

  /// Get current timestamp in seconds (Unix epoch)
  int getCurrentTimestamp() {
    return (DateTime.now().millisecondsSinceEpoch / 1000).floor();
  }

  /// Check if timestamp is within acceptable window
  bool isTimestampValid(int timestamp, {int windowSeconds = 30}) {
    final now = getCurrentTimestamp();
    final drift = (timestamp - now).abs();
    return drift <= windowSeconds;
  }

  // ── Rate Limiting ─────────────────────────────────────────────────

  /// Check rate limiting for endpoint
  /// Returns true if request is allowed
  bool checkRateLimit(String endpoint, {
    int maxRequestsPerWindow = 10,
    int windowSeconds = 60,
  }) {
    final key = 'ratelimit_$endpoint';
    final lastRequests = prefs.getStringList(key) ?? [];
    final now = DateTime.now().millisecondsSinceEpoch;
    
    // Remove old requests outside window
    final recentRequests = lastRequests
        .map(int.parse)
        .where((t) => now - t < windowSeconds * 1000)
        .toList();
    
    if (recentRequests.length >= maxRequestsPerWindow) {
      debugPrint('Rate limit exceeded for $endpoint');
      return false;
    }
    
    // Add current request
    recentRequests.add(now);
    prefs.setStringList(key, recentRequests.map((t) => t.toString()).toList());
    return true;
  }

  /// Clear rate limit for endpoint (admin function)
  void clearRateLimit(String endpoint) {
    prefs.remove('ratelimit_$endpoint');
  }

  /// Get remaining requests for endpoint before rate limit
  int getRemainingRequests(String endpoint, {int maxPerWindow = 10}) {
    final key = 'ratelimit_$endpoint';
    final lastRequests = prefs.getStringList(key) ?? [];
    final now = DateTime.now().millisecondsSinceEpoch;
    
    final recentCount = lastRequests
        .map(int.parse)
        .where((t) => now - t < 60000)  // 1 minute window
        .length;
    
    return maxPerWindow - recentCount;
  }
}

// ─────────────────────────────────────────────
// Nonce Manager
// ─────────────────────────────────────────────

class NonceManager {
  static const int NONCE_SIZE = 16;  // 128-bit nonce
  static const int NONCE_EXPIRY_MS = 30000;  // 30 seconds
  
  final Map<String, int> _usedNonces = {};  // nonce -> timestamp
  late final Random _random;

  NonceManager() {
    _random = Random.secure();
  }

  /// Generate a cryptographically secure random nonce
  String generateNonce() {
    final bytes = List<int>.generate(NONCE_SIZE, (_) => _random.nextInt(256));
    return bytes.map((b) => b.toRadixString(16).padLeft(2, '0')).join();
  }

  /// Validate that nonce hasn't been used and isn't expired
  bool isValidNonce(String nonce) {
    final now = DateTime.now().millisecondsSinceEpoch;
    
    // Check if already used
    if (_usedNonces.containsKey(nonce)) {
      final usedAt = _usedNonces[nonce]!;
      if (now - usedAt < NONCE_EXPIRY_MS) {
        debugPrint('Nonce replay detected: $nonce');
        return false;  // Still in expiry window
      }
    }
    
    // Valid - mark as used
    _usedNonces[nonce] = now;
    
    // Cleanup old nonces (older than 2x expiry)
    _usedNonces.removeWhere((_, timestamp) => now - timestamp > NONCE_EXPIRY_MS * 2);
    
    return true;
  }

  /// Clear all used nonces
  void clear() {
    _usedNonces.clear();
  }
}

// ─────────────────────────────────────────────
// Secure Command Builder
// ─────────────────────────────────────────────

class SecureCommandBuilder {
  final String deviceId;
  final String token;
  final SecurityManager securityManager;
  
  SecureCommandBuilder({
    required this.deviceId,
    required this.token,
    required this.securityManager,
  });

  /// Build a secure command payload with nonce and timestamp
  /// Call fetchNonce() first via /api/nonce endpoint
  Map<String, dynamic>? buildSecureCommand(
    int command, {
    required String nonce,
  }) {
    // Validate nonce is recent
    if (!securityManager.validateNonceResponse(nonce)) {
      debugPrint('Invalid nonce: $nonce');
      return null;
    }

    final timestamp = securityManager.getCurrentTimestamp();
    
    // Check timestamp validity
    if (!securityManager.isTimestampValid(timestamp)) {
      debugPrint('Timestamp out of window: $timestamp');
      return null;
    }

    return {
      'device_id': deviceId.trim(),
      'token': token.trim(),
      'command': command,
      'nonce': nonce,
      'timestamp': timestamp,
    };
  }
}