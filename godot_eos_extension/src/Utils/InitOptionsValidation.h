#pragma once

#include <godot_cpp/variant/string.hpp>

namespace godot {

// EOS_PLATFORM_OPTIONS_ENCRYPTIONKEY_LENGTH: exactly 64 hexadecimal characters.
constexpr int kEosEncryptionKeyHexLength = 64;

// Only a 64-character hexadecimal encryption_key is valid.
// Returns an empty String on success, or a user-facing error that names encryption_key.
String ValidateEncryptionKey(const String& encryption_key);

} // namespace godot
