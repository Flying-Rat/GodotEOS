#pragma once

#include "../Utils/ISubsystem.h"
#include <eos_userinfo_types.h>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace godot {

/**
 * @brief Interface for User Info subsystem.
 *
 * Provides centralized user information query functionality for all EOS users.
 * Handles querying and caching user display names, nicknames, and other profile data.
 */
class IUserInfoSubsystem : public ISubsystem {
public:
    virtual ~IUserInfoSubsystem() = default;

    /**
     * @brief Query user information from EOS.
     * 
     * Initiates an asynchronous query to fetch user information.
     * After the query completes, use GetCachedUserInfo() to retrieve the data.
     * 
     * @param local_user_id The EOS_EpicAccountId of the local user making the request
     * @param target_user_id The EOS_EpicAccountId of the user to query
     * @return true if query was initiated successfully
     */
    virtual bool QueryUserInfo(EOS_EpicAccountId local_user_id, EOS_EpicAccountId target_user_id) = 0;

    /**
     * @brief Get cached user information.
     * 
     * Returns cached user info if available. Does not trigger a query.
     * 
     * @param local_user_id The EOS_EpicAccountId of the local user
     * @param target_user_id The EOS_EpicAccountId of the user to get info for
     * @return Dictionary containing user info (display_name, nickname, country, etc.) or empty if not cached
     */
    virtual Dictionary GetCachedUserInfo(EOS_EpicAccountId local_user_id, EOS_EpicAccountId target_user_id) = 0;

    /**
     * @brief Get user's display name.
     * 
     * Convenience method to get just the display name from cached data.
     * Falls back to nickname if display name is not available.
     * Returns empty string if user info is not cached.
     * 
     * @param local_user_id The EOS_EpicAccountId of the local user
     * @param target_user_id The EOS_EpicAccountId of the user to get name for
     * @return The user's display name, nickname, or empty string if not cached
     */
    virtual String GetUserDisplayName(EOS_EpicAccountId local_user_id, EOS_EpicAccountId target_user_id) = 0;

    /**
     * @brief Check if user info is cached.
     * 
     * @param local_user_id The EOS_EpicAccountId of the local user
     * @param target_user_id The EOS_EpicAccountId of the user to check
     * @return true if user info is cached and available
     */
    virtual bool IsUserInfoCached(EOS_EpicAccountId local_user_id, EOS_EpicAccountId target_user_id) = 0;

    /**
     * @brief Clear all cached user information.
     */
    virtual void ClearCache() = 0;
};

} // namespace godot
