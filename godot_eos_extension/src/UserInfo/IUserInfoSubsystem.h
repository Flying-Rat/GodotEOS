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
    virtual bool QueryUserInfo(EOS_EpicAccountId target_user_id) = 0;

    /**
     * @brief Get cached user information.
     * 
     * Returns cached user info if available. Does not trigger a query.
     * 
     * @param target_user_id The EOS_EpicAccountId of the user to get info for
     * @return Dictionary containing user info (display_name, nickname, country, etc.) or empty if not cached
     */
    virtual Dictionary GetCachedUserInfo(EOS_EpicAccountId target_user_id) = 0;

    /**
     * @brief Get user's display name.
     * 
     * Convenience method to get just the display name from cached data.
     * Falls back to nickname if display name is not available.
     * Returns empty string if user info is not cached.
     * 
     * @param target_user_id The EOS_EpicAccountId of the user to get name for
     * @return The user's display name, nickname, or empty string if not cached
     */
    virtual String GetUserDisplayName(EOS_EpicAccountId target_user_id) = 0;

    /**
     * @brief Check if user info is cached.
     * 
     * @param target_user_id The EOS_EpicAccountId of the user to check
     * @return true if user info is cached and available
     */
    virtual bool IsUserInfoCached(EOS_EpicAccountId target_user_id) = 0;

    /**
     * @brief Clear all cached user information.
     */
    virtual void ClearCache() = 0;

    /**
     * @brief Set the callback for user info query completion.
     * 
     * The callback will be called with (bool success, Dictionary user_info)
     * where user_info contains the queried user information.
     * 
     * @param callback The callable to invoke when user info query completes
     */
    virtual void SetUserInfoQueryCallback(const Callable& callback) = 0;

    /**
     * @brief Set the callback for user cache updates.
     * 
     * The callback will be called with (String epic_account_id, Dictionary user_data)
     * whenever user cache data is updated (e.g., Product ID found).
     * 
     * @param callback The callable to invoke when user cache is updated
     */
    virtual void SetUserCacheUpdateCallback(const Callable& callback) = 0;

    // Phase 1: User cache methods

    /**
     * @brief Notify subsystem that player info has been updated.
     * 
     * Called when user information changes (login, friend query, etc.).
     * Triggers caching and automatic Product ID resolution if needed.
     * 
     * @param epic_id The EOS_EpicAccountId of the user
     * @param product_id Optional Product User ID (if known)
     * @param is_local_user True if this is the authenticated user
     */
    virtual void UpdateUserCache(EOS_EpicAccountId epic_id, EOS_ProductUserId product_id = nullptr, bool is_local_user = false) = 0;

    /**
     * @brief Get cached Product User ID for a user.
     * 
     * @param epic_id The EOS_EpicAccountId to lookup
     * @return Product User ID as String, or empty if not cached
     */
    virtual String GetUserProductId(EOS_EpicAccountId epic_id) = 0;

    /**
     * @brief Check if Product ID is cached for a user.
     * 
     * @param epic_id The EOS_EpicAccountId to check
     * @return true if Product ID is available in cache
     */
    virtual bool IsProductIdCached(EOS_EpicAccountId epic_id) = 0;

    /**
     * @brief Get full user cache entry.
     * 
     * @param epic_id The EOS_EpicAccountId to lookup
     * @return Dictionary with all cached user data
     */
    virtual Dictionary GetCachedUserData(EOS_EpicAccountId epic_id) = 0;

    /**
     * @brief Force re-query of Product ID for a user.
     * 
     * Resets the query flag and initiates a new Product ID query,
     * allowing manual re-querying even after previous failures.
     * 
     * @param epic_id The EOS_EpicAccountId to query
     * @return true if query was initiated successfully
     */
    virtual bool QueryProductId(EOS_EpicAccountId epic_id) = 0;

    /**
     * @brief Retry Product ID queries for all cached friends.
     * 
     * Called when Connect login completes to retry any failed Product ID queries
     * for friends that were cached before Connect login was available.
     */
    virtual void RetryFriendProductIdQueries() = 0;
};

} // namespace godot
