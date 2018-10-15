//
// Copyright (c) 2018 SNC Limited
//

#include "snctbnescrow.hpp"

namespace snct {


	void snctbnescrow::sendbonus(uint64_t from_user_id,
		account_name to_account,
		string memo)
	{
		require_auth(_self);

		auto iter = escrows.find(from_user_id);
		eosio_assert(iter != escrows.end(), "No tokens to withraw");
		eosio_assert(iter->quantityTotal.amount > 0, "must withraw positive quantity");
		eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");



		if (iter->withdrawNumber <12)
		{
			escrows.modify(iter, _self, [&](auto& row) {
				row.quantityTotal -=  iter->quantityMonthly;
				row.withdrawNumber+=1;
			});
		}
		else
		{
			escrows.erase(iter);
		}



		if(iter->quantityTotal.amount <= iter->quantityMonthly.amount){
			if(to_account ==N(snctescrow11)){
				action(permission_level{_self, N(active)}, N(sncttokens11), N(transfer),
					make_tuple(_self,to_account,iter->quantityTotal,memo)).send();

			}else{
				action(permission_level{ _self, N(active) }, N(sncttokens11), N(transfer),
					make_tuple(_self, to_account, iter->quantityTotal, string("snctescrow11 to user account"))).send();
				escrows.erase(iter);}

		}
		else{
			if(to_account ==N(snctescrow11)){
				action(permission_level{_self, N(active)}, N(sncttokens11), N(transfer),
					make_tuple(_self,to_account,iter->quantityTotal,memo)).send();
			}else{
				action(permission_level{ _self, N(active) }, N(sncttokens11), N(transfer),
					make_tuple(_self, to_account, iter->quantityMonthly, string("snctescrow11 to user account"))).send();}
		}
	}

	void snctbnescrow::apply(const account_name contract, const account_name act) {

		if (act == N(transfer)) {
			transferReceived(unpack_action_data<currency::transfer>(), contract);
			return;
		}

		auto &thiscontract = *this;

		switch (act) {
			EOSIO_API(snctbnescrow, (sendbonus))
		};
	}


	void snctbnescrow::transferReceived(const currency::transfer &transfer, account_name code) {
		eosio_assert(static_cast<uint32_t>(transfer.quantity.symbol == S(4, SNCT)), "only snct token allowed");
		eosio_assert(static_cast<uint32_t>(code == N(sncttokens11)), "needs to come from sncttokens11");
		eosio_assert(static_cast<uint32_t>(transfer.memo.length() > 0), "needs a memo with the user id");
		eosio_assert(static_cast<uint32_t>(transfer.quantity.is_valid()), "invalid transfer");
		eosio_assert(static_cast<uint32_t>(transfer.quantity.amount > 0), "must be positive quantity!");
		eosio_assert(transfer.from == N(snctprvsales)
			, "Only snct accounts allowed");

		uint64_t user_id = std::stoull(transfer.memo);
		auto iter = escrows.find(user_id);

		if (iter != escrows.end())
		{
			escrows.modify(iter, _self, [&](auto& row) {
				row.quantityTotal += transfer.quantity;
				asset qm = transfer.quantity/12;
				row.quantityMonthly  += qm;
			});
		}
		else
		{
			escrows.emplace(_self, [&](auto& row) {
				row.user_id = user_id;
				row.quantityTotal = transfer.quantity;
				asset qm = transfer.quantity/12;
				row.quantityMonthly  = qm;
			});
		}


		if (transfer.to != _self) {
			return;
		}
	}

} /// namespace snct

extern "C" {

	using namespace snct;
	using namespace eosio;

	void apply(uint64_t receiver, uint64_t code, uint64_t action) {
		auto self = receiver;
		snctbnescrow contract(self);
		contract.apply(code, action);
		eosio_exit(0);
	}
}
