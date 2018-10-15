//
// Copyright (c) 2018 SNC Limited
//

#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/currency.hpp>

#include <string>

using namespace eosio;

namespace snct {

	using std::string;

	class snctbnescrow : public contract {
	public:
		snctbnescrow(account_name self) :contract(self), escrows(_self, _self) {}

		void sendbonus(uint64_t from_user_id,
			account_name to_account,
			string       memo);

		void apply(const account_name contract, const account_name act);

		void transferReceived(const currency::transfer &transfer, account_name code);


		// @abi table
		struct escrow {
			uint64_t user_id;
			asset quantityTotal;
			asset    quantityMonthly;
			uint32_t withdrawNumber = 0;

			uint64_t primary_key() const { return user_id; };
		};


		multi_index<N(escrow), escrow> escrows;

	};

} /// namespace snct
