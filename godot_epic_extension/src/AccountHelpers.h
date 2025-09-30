// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <eos_sdk.h>
#include <godot_cpp/variant/string.hpp>
#include "StringUtils.h"

/**
* Helper utilities
*/
class FAccountHelpers
{
public:
	/**
	* Constructor
	*/
	FAccountHelpers() noexcept(false);

	/**
	* No copying or copy assignment allowed for this class.
	*/
	FAccountHelpers(FAccountHelpers const&) = delete;
	FAccountHelpers& operator=(FAccountHelpers const&) = delete;

	/**
	* Destructor
	*/
	virtual ~FAccountHelpers();

	/**
	* Utility to convert account id to a string
	*
	* @param InAccountId - Account id to convert
	*
	* @return String representing account id. Returns string representation of error in case of bad account id.
	*/
	static godot::String EpicAccountIDToString(EOS_EpicAccountId InAccountId);
	static godot::String ProductUserIDToString(EOS_ProductUserId InAccountId);

	/**
	* Utility to build Epic Account ID from string
	*/
	static EOS_EpicAccountId EpicAccountIDFromString(const char* AccountString);

	/**
	* Utility to build Product User ID from string
	*/
	static EOS_ProductUserId ProductUserIDFromString(const char* ProductUserIdString);
};

template <typename T>
struct TValidateAccount
{
};

template <>
struct TValidateAccount<EOS_ProductUserId>
{
	static bool IsValid(const EOS_ProductUserId ProductUserId)
	{
		return EOS_ProductUserId_IsValid(ProductUserId);
	}

	static std::wstring ToString(const EOS_ProductUserId ProductUserId)
	{
		godot::String str = FAccountHelpers::ProductUserIDToString(ProductUserId);
		return FStringUtils::Widen(str.utf8().get_data());
	}
};

template <>
struct TValidateAccount<EOS_EpicAccountId>
{
	static bool IsValid(const EOS_EpicAccountId AccountId)
	{
		return EOS_EpicAccountId_IsValid(AccountId);
	}

	static std::wstring ToString(const EOS_EpicAccountId AccountId)
	{
		godot::String str = FAccountHelpers::EpicAccountIDToString(AccountId);
		return FStringUtils::Widen(str.utf8().get_data());
	}
};

/**
 * Simple wrapper around account ID. Allows easy conversion and adds handy operators.
 */
template<class TAccountType>
struct TEpicAccountId
{
	/**
	* Construct wrapper from account id.
	*/
	TEpicAccountId(TAccountType InAccountId) : AccountId(InAccountId) {}
	TEpicAccountId() = default;
	TEpicAccountId(const TEpicAccountId&) = default;
	TEpicAccountId& operator=(const TEpicAccountId&) = default;

	bool operator==(const TEpicAccountId& Other) const
	{
		return AccountId == Other.AccountId;
	}

	bool operator!=(const TEpicAccountId& Other) const
	{
		return !(this->operator==(Other));
	}

	bool operator<(const TEpicAccountId& Other) const
	{
		return AccountId < Other.AccountId;
	}

	/**
	* Checks if account ID is valid.
	*/
	operator bool() const { return IsValid(); }

	/**
	* Easy conversion to EOS account ID.
	*/
	operator TAccountType() const
	{
		return AccountId;
	}

	/**
	* Prints out account ID as hex.
	*/
	std::wstring ToString() const
	{
		return TValidateAccount<TAccountType>::ToString(AccountId);
	}

	/**
	* True if account id  is valid
	*/
	bool IsValid() const 
	{ 
		return TValidateAccount<TAccountType>::IsValid(AccountId);
	};

	/** EOS Account Id */
	TAccountType AccountId = nullptr;
};

using FEpicAccountId = TEpicAccountId<EOS_EpicAccountId>;
using FProductUserId = TEpicAccountId<EOS_ProductUserId>;
