//
// Copyright (c) 2018 SNC Limited
//

#include "snctprvsales.hpp"

namespace snct {


	void snctprvsales::setprvsales(uint32_t salesstart, uint32_t salesend, bool suspend)
	{
		require_auth(_self);
		prvsalescons(_self, _self).set(prvsalescon{salesstart,  salesend, suspend}, _self);
	}


	void snctprvsales::setprvphase(string name, uint32_t phasestart, uint32_t phaseend, double bonus_rate, double exrate){
		require_auth(_self);

		//TODO do assertion about the range of exchangerate
		uint32_t bonus = bonus_rate;
		uint64_t exchangerate = exrate * 10000;
		eosio_assert(bonus <=100,"bonus not in range");
		phases(_self,_self).set(phase{name, phasestart, phaseend, bonus, exchangerate}, _self);
		eosio_assert(phases(_self, _self).exists(), "phases in setprvphase account not configured");
	}

	void snctprvsales::addauser(uint64_t prvsales_id, name user)
	{
		require_auth(_self);

		auto iter = allowedprvs.find(prvsales_id);

		if (iter == allowedprvs.end()) {

			allowedprvs.emplace(_self, [&](auto& row) {
				row.prvsales_id = prvsales_id;
				row.user = user;
			});
		}
		else{
			allowedprvs.modify(iter, _self, [&](auto& row) {
				row.user = user;
			});
		}
	}

	void snctprvsales::addausers(vector<allowedprv> users)
	{
		require_auth(_self);


		for (auto& n : users)
		{
			auto iter = allowedprvs.find(n.prvsales_id);

			if (iter == allowedprvs.end()) {

				allowedprvs.emplace(_self, [&](auto& row) {
					row.prvsales_id = n.prvsales_id;
					row.user = n.user;
				});
			}
			else
			{
				allowedprvs.modify(iter, _self, [&](auto& row) {
					row.user = n.user;
				});
			}
		}
	}




	void snctprvsales::delauser(uint64_t prvsales_id)
	{
		require_auth(_self);


		auto iter = allowedprvs.find(prvsales_id);

		if (iter != allowedprvs.end()) {
			allowedprvs.erase(iter);
		}
	}

	void snctprvsales::delall()
	{
		require_auth(_self);

		auto iter = allowedprvs.begin();

		while (iter != allowedprvs.end())
		{
			iter = allowedprvs.erase(iter);
		}
	}


	void snctprvsales::addpurchase(uint64_t purchaser, asset snctbought, asset eospaid)
	{
		require_auth(_self);


		purchases p(_self, _self);
		auto iter = p.find(purchaser);


		if (iter == p.end()) {
			p.emplace(_self, [&](auto& row) {
				row.purchaser = purchaser;
				row.snctbought = snctbought;
				row.eospaid = eospaid;

			});
		}
		else {
			p.modify(iter, _self, [&](auto& row) {
				row.snctbought += snctbought;
				row.eospaid += eospaid;
			});
		}
	}


	void snctprvsales::setsuspend(bool suspend){
		require_auth(_self);


		auto salescon = prvsalescons(_self, _self).get();
		uint32_t salesstart = salescon.salesstart;
		uint32_t salesend = salescon.salesend;

		prvsalescons(_self, _self).set(prvsalescon{salesstart,  salesend, suspend}, _self);
	}


	void snctprvsales::apply(const account_name contract, const account_name act) {

		if (act == N(transfer)) {
			transferReceived(unpack_action_data<currency::transfer>(), contract);
			return;
		}

		auto &thiscontract = *this;

		switch (act) {
			EOSIO_API(snctprvsales, (setprvsales)(setprvphase)(addauser)(addausers)(delauser)(delall)(returntokens)(transinput)(setsuspend)(addpurchase))
		};
	}



	void snctprvsales::returntokens(uint64_t purchaser) {
		require_auth(_self);


		purchases p(_self, _self);
		auto iterPurchase = p.find(purchaser);

		auto username = allowedprvs.find(purchaser);

		eosio_assert(iterPurchase != p.end(), "no tokens to return");

		//sending back EOS TOKENS
		action(permission_level{ _self, N(active) }, N(eosio.token), N(transfer),
			make_tuple(_self, username->user, iterPurchase->eospaid, string("Return of EOS tokens after unsuccessful pubsales"))).send();

		//deleting the row in the table
		p.erase(iterPurchase);
	}


	//userAccount will be escrow account
	void snctprvsales::transinput(uint64_t user_id, asset tokenAmount) {
		require_auth(_self);


		eosio_assert(static_cast<uint32_t>(user_id > 0), "needs a memo with the name");
		eosio_assert(static_cast<uint32_t>(tokenAmount.symbol == S(4, SNCT)), "only SNCT token allowed");
		eosio_assert(static_cast<uint32_t>(tokenAmount.is_valid()), "invalid transfer");



		auto salescon = prvsalescons(_self, _self).get();
		uint32_t salesstart = salescon.salesstart;
		uint32_t salesend = salescon.salesend;
		uint32_t nowTime = now();
		bool isSuspend = salescon.suspend;


		//dates within ICO
		eosio_assert(salesstart < nowTime, "Sales has not started");
		eosio_assert(nowTime  < salesend, "Sales has ended");
		eosio_assert(isSuspend==false, "all the transaction is suspended");



		uint64_t ico_id = user_id;
		auto iterUser = allowedprvs.find(ico_id);
		//make sure the user exists
		eosio_assert(iterUser!=allowedprvs.end(), "the user was not added! it is not whitelist customer");

		//the sending account must match the one registered in our app - memo should contain the ico_id that can be found in the account settings
		account_name userAccount = iterUser->user;

		if(userAccount== N(snctescrow11)){
			action(permission_level{_self, N(active)}, N(sncttokens11), N(transfer),make_tuple(_self,N(snctescrow11),tokenAmount,std::to_string(user_id))).send();
		}
		else{
			action(permission_level{ _self, N(active) }, N(sncttokens11), N(transfer),
				make_tuple(_self, userAccount, tokenAmount, string("Thank you for taking part in our pubsales!"))).send();
		}




		//send bonus
		auto phasescon = phases(_self,_self).get();
		uint32_t bonusPercent = phasescon.bonus;
		asset bonussnctamount = tokenAmount*bonusPercent;
		bonussnctamount = bonussnctamount/100;



		if(bonusPercent>0){
			action(permission_level{_self, N(active)}, N(sncttokens11), N(transfer),make_tuple(_self,N(snctbnescrow),bonussnctamount,std::to_string(user_id))).send();

		}

		string sym = "EOS";
		symbol_type symbolvalue = string_to_symbol(4,sym.c_str());
		asset zeroEOS;
		zeroEOS.amount = 0;
		zeroEOS.symbol = symbolvalue;
		//asset zeroEOS = asset(0, S(4, EOS));
		addpurchase(ico_id,tokenAmount, zeroEOS);
	}







	void snctprvsales::transferReceived(const currency::transfer &transfer, const account_name code) {
		if(transfer.from == N(sncttokens11)){
			return;
		}
		else
		{
			if (transfer.to != _self) {
				return;
			}
			eosio_assert(phases(_self, _self).exists(), "phases not configured");
			eosio_assert(prvsalescons(_self, _self).exists(), "prvsales not configured");
			eosio_assert(static_cast<uint32_t>(code == N(eosio.token)), "needs to come from eosio.token");
			eosio_assert(static_cast<uint32_t>(code == N(eosio.token)), "needs to come from eosio.token");
			eosio_assert(static_cast<uint32_t>(transfer.memo.length() > 0), "needs a memo with the name");
			eosio_assert(static_cast<uint32_t>(transfer.quantity.symbol == S(4, EOS)), "only EOS token allowed");
			eosio_assert(static_cast<uint32_t>(transfer.quantity.is_valid()), "invalid transfer");
			//eosio_assert(static_cast<uint32_t>(transfer.quantity.amount >= 1000), "must be at least 0.1 EOS");
			auto prvsalescon = prvsalescons(_self, _self).get();
			auto phasecon = phases(_self, _self).get();
			uint32_t salesstart = prvsalescon.salesstart;
			uint32_t salesend = prvsalescon.salesend;
			uint32_t nowTime = now();

			//dates within prvsales
			eosio_assert(salesstart < nowTime, "prvsales has not started");
			eosio_assert(nowTime  < salesend, "prvsales has ended");

			uint64_t USDrate = phasecon.exchangerate;

			uint64_t prvsales_id = std::stoull(transfer.memo);
			auto iterUser = allowedprvs.find(prvsales_id);
			eosio_assert(iterUser!=allowedprvs.end(), "The user is not in the whitelist!");

			//the sending account must match the one registered in our app - memo should contain the prvsales_id that can be found in the account settings
			require_auth(iterUser->user);

			asset receivedEOS = transfer.quantity;
			uint64_t EOSamount = receivedEOS.amount;
			uint64_t priceInUSDmultiplyBy10000 = 1087;
			uint64_t snctamount = (EOSamount * USDrate) / priceInUSDmultiplyBy10000;

			string sym = "SNCT";
			symbol_type symbolvalue = string_to_symbol(4,sym.c_str());
			asset sncttoSend;
			sncttoSend.amount = snctamount;
			sncttoSend.symbol = symbolvalue;
			//sending snct TOKENS
			action(permission_level{ _self, N(active) }, N(sncttokens11), N(transfer),
				make_tuple(_self, transfer.from, sncttoSend, string("Thank you for taking part in our prvsales!!"))).send();

			//adding to purchase table
			SEND_INLINE_ACTION(*this, addpurchase, { _self,N(active) }, { prvsales_id, sncttoSend, receivedEOS});

			//send bonus
			uint32_t bonusPercent = phasecon.bonus;
			uint64_t bonussnctamount = snctamount*bonusPercent;
			bonussnctamount = bonussnctamount/100;


			if(bonusPercent>0){
				asset totalBonus;
				totalBonus.amount = bonussnctamount;
				totalBonus.symbol = symbolvalue;
				action(permission_level{_self, N(active)}, N(sncttokens11), N(transfer),make_tuple(_self,N(snctbnescrow),totalBonus,transfer.memo)).send();

			}



		}


	}

} /// namespace snct

extern "C" {

	using namespace snct;
	using namespace eosio;

	void apply(uint64_t receiver, uint64_t code, uint64_t action) {
		auto self = receiver;
		snctprvsales contract(self);
		contract.apply(code, action);
		eosio_exit(0);
	}
}
