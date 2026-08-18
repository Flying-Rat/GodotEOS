#include "InitOptionsValidation.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {

static bool is_hex_char(char32_t c) {
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

String ValidateEncryptionKey(const String& encryption_key) {
	if (encryption_key.length() != kEosEncryptionKeyHexLength) {
		return vformat(
				"Invalid encryption_key: length is %d; EOS requires exactly %d hexadecimal characters (0-9, a-f, A-F).",
				encryption_key.length(),
				kEosEncryptionKeyHexLength);
	}

	for (int i = 0; i < encryption_key.length(); ++i) {
		if (!is_hex_char(encryption_key[i])) {
			return vformat(
					"Invalid encryption_key: character at index %d is not hexadecimal (0-9, a-f, A-F).",
					i);
		}
	}

	return String();
}

} // namespace godot
