#pragma once

#include <godot_cpp/variant/string.hpp>

namespace godot {

// EOS_PLATFORM_OPTIONS_ENCRYPTIONKEY_LENGTH: 64 hex characters, or omit the key.
constexpr int kEosEncryptionKeyHexLength = 64;

// Empty string is valid (pass nullptr to EOS). Non-empty keys must be exactly 64 hex chars.
// Returns an empty String on success, or a user-facing error that names encryption_key.
String ValidateEncryptionKey(const String& encryption_key);

} // namespace godot
