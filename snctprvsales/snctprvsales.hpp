//
// Copyright (c) 2018 SNC Limited
//

#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/currency.hpp>
#include "../sncttokens11/sncttokens11.hpp"
#include <string>

using namespace eosio;

namespace snct {

	using std::string;

	class snctprvsales : public contract {
	public:
		snctprvsales(account_name self) :contract(self), allowedprvs(_self, _self) {}

		// @abi table
		struct allowedprv {
			uint64_t prvsales_id;
			name user;

			uint64_t primary_key() const { return prvsales_id; }

			EOSLIB_SERIALIZE(allowedprv, (prvsales_id)(user))
		};


		// @abi table purchase i64
		struct purchase {
			//purchaser_id - same as allowedprvs
			uint64_t purchaser;
			asset snctbought;
			asset eospaid;

			uint64_t primary_key()const { return purchaser; }

			EOSLIB_SERIALIZE(purchase, (purchaser)(snctbought)(eospaid))
		};



		struct prvsalescon {
			uint32_t salesstart;
			uint32_t salesend;
			bool suspend;
		};

		struct phase{
			string name;
			uint32_t phasestart;
			uint32_t phaseend;
			//discount in int e.g) 10% discount - 10
			uint32_t bonus;
			uint64_t exchangerate;

		};

		// @abi action
		void setprvsales(uint32_t salesstart, uint32_t salesend, bool suspend);

		// @abi action
		void setprvphase(string name, uint32_t phasestart, uint32_t phaseend, double bonus, double exchangerate);

		// @abi action
		void addauser(uint64_t prvsales_id, name user);

		// @abi action
		void addausers(vector<allowedprv> users);

		// @abi action
		void delauser(uint64_t prvsales_id);

		// @abi action
		void delall();


		// @abi action
		void returntokens(uint64_t purchaser_id);

		// @abi action
		void addpurchase(uint64_t purchaser, asset snctbought, asset eospaid);


		void setsuspend(bool suspend);

		// @abi action
		void transinput(uint64_t user_id, asset tokenAmount);



		void apply(const account_name contract, const account_name act);


		void transferReceived(const currency::transfer &transfer, const account_name code);



		//typedef singleton<N(config), config> configs;
		typedef singleton<N(prvsalescon), prvsalescon> prvsalescons;
		typedef singleton<N(phase), phase> phases;

		multi_index<N(allowedprv), allowedprv> allowedprvs;

		typedef multi_index<N(purchase), purchase> purchases;

	}; /// namespace snct
}
