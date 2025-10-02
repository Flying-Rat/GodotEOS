#pragma once

#include "../Utils/ISubsystem.h"
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {

/**
 * @brief Friends subsystem interface.
 *
 * Handles EOS friends functionality including querying friends list,
 * friend information, and friend status management.
 */
class IFriendsSubsystem : public ISubsystem {
public:
    /**
     * @brief Query the current user's friends list from EOS.
     * @return true if query initiated successfully.
     */
    virtual bool QueryFriends() = 0;

    /**
     * @brief Get the cached friends list.
     * @return Array of friend dictionaries containing friend information.
     */
    virtual Array GetFriendsList() const = 0;

    /**
     * @brief Get information about a specific friend.
     * @param friend_id The Epic Account ID of the friend as a string.
     * @return Dictionary containing friend information, or empty dict if not found.
     */
    virtual Dictionary GetFriendInfo(const String& friend_id) const = 0;

    /**
     * @brief Query detailed information about a specific friend.
     * @param friend_id The Epic Account ID of the friend as a string.
     * @return true if query initiated successfully.
     */
    virtual bool QueryFriendInfo(const String& friend_id) = 0;

    /**
     * @brief Query detailed information for all friends.
     * @return true if queries initiated successfully.
     */
    virtual bool QueryAllFriendsInfo() = 0;

    /**
     * @brief Set a callback for friends list query completion.
     * @param callback Callable to invoke when friends query completes.
     */
    virtual void SetFriendsQueryCallback(const Callable& callback) = 0;

    /**
     * @brief Set a callback for friend info query completion.
     * @param callback Callable to invoke when friend info query completes.
     */
    virtual void SetFriendInfoQueryCallback(const Callable& callback) = 0;
};

} // namespace godot