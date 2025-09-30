#pragma once

#include "IFriendsSubsystem.h"
#include "../eos_sdk/Include/eos_friends_types.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace godot {

/**
 * @brief Friends subsystem implementation.
 *
 * Manages EOS friends functionality including querying friends list,
 * friend information, and handling friend status updates.
 */
class FriendsSubsystem : public IFriendsSubsystem {
public:
    FriendsSubsystem();
    ~FriendsSubsystem() override;

    // ISubsystem interface
    bool Init() override;
    void Tick(float delta_time) override;
    void Shutdown() override;
    const char* GetSubsystemName() const override { return "Friends"; }

    // IFriendsSubsystem interface
    bool QueryFriends() override;
    Array GetFriendsList() const override;
    Dictionary GetFriendInfo(const String& friend_id) const override;
    bool QueryFriendInfo(const String& friend_id) override;
    bool QueryAllFriendsInfo() override;
    void SetFriendsQueryCallback(const Callable& callback) override;
    void SetFriendInfoQueryCallback(const Callable& callback) override;

private:
    Array friends_list;
    bool friends_cached;

    // Callback callables
    Callable friends_query_callback;
    Callable friend_info_query_callback;

    // Static callback for friends query
    static void EOS_CALL on_friends_query_complete(const EOS_Friends_QueryFriendsCallbackInfo* data);

    // Helper methods
    void update_friends_list();
    Dictionary create_friend_info_dict(EOS_EpicAccountId friend_id) const;
};

} // namespace godot