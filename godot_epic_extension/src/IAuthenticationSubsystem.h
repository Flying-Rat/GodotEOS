#pragma once

#include "ISubsystem.h"
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {

/**
 * @brief Authentication subsystem interface.
 *
 * Handles EOS authentication including login, logout, and user management.
 */
class IAuthenticationSubsystem : public ISubsystem {
public:
    /**
     * @brief Login with the specified authentication method.
     * @param login_type Type of login ("epic_account", "device_id", "exchange_code", etc.)
     * @param credentials Dictionary of credentials specific to the login type.
     * @return true if login request initiated successfully.
     */
    virtual bool Login(const String& login_type, const Dictionary& credentials) = 0;

    /**
     * @brief Logout the current user.
     * @return true if logout request initiated successfully.
     */
    virtual bool Logout() = 0;

    /**
     * @brief Check if user is currently logged in.
     * @return true if user is authenticated.
     */
    virtual bool IsLoggedIn() const = 0;

    /**
     * @brief Get the Product User ID.
     * @return Product User ID string, or empty string if not logged in.
     */
    virtual String GetProductUserId() const = 0;

    /**
     * @brief Get the Epic Account ID.
     * @return Epic Account ID string, or empty string if not logged in.
     */
    virtual String GetEpicAccountId() const = 0;

    /**
     * @brief Get the current user's display name.
     * @return User display name, or empty string if not logged in.
     */
    virtual String GetDisplayName() const = 0;

    /**
     * @brief Get the current login status.
     * @return Login status as integer (EOS_ELoginStatus enum values).
     */
    virtual int GetLoginStatus() const = 0;

    /**
     * @brief Set a callback for login completion events.
     * @param callback Callable to invoke when login completes.
     */
    virtual void SetLoginCallback(const Callable& callback) = 0;
};

} // namespace godot