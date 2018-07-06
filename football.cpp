// create by kaiz(ishotoli@aliyun.com)
// 2018-07-07
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/asset.hpp>
#include <string>
#include <stdlib.h>
#include <map>
using namespace eosio;

class football : public eosio::contract {
  const float rate = 0.9F;
  public:
      hello(account_name self)
      :eosio::contract(self),
      games(_self, _self),
      options(_self, _self),
      offers(_self, _self)
      {}

      /// @abi action 
      void ver() {
        print( "Hello32, ", name{_self}, ", Now: ", now() );
      }

      /// @abi action
      void setgame( const uint64_t id, const std::string& game_name, const account_name creator, const uint32_t deadline ) {
        require_auth( creator );
        eosio_assert( creator == _self, "only owner can call this action");
        eosio_assert(game_name.length() > 0, "game name cannot be null");
        eosio_assert(deadline > now(), "deadline must greater than now");

        auto game_itr = games.find(id);
        if (game_itr == games.end()) {
          games.emplace(_self, [&](auto& new_game){
            new_game.id = id;
            new_game.name = game_name;
            new_game.owner = creator;
            new_game.deadline.utc_seconds = deadline;
            new_game.create_time.utc_seconds = now();
          });
        } else {
          games.modify(game_itr, 0, [&](auto& game){
            game.name = game_name;
            game.deadline.utc_seconds = deadline;
          });
        }
        print( "join game : ", game_name);
      }

      /// @abi action
      void getgames( const account_name user ) {
        require_auth( user );
        for(auto game_itr = games.begin(); game_itr!=games.end(); game_itr++) {
          print(" | id:", game_itr->id, "/name:", game_itr->name, "/deadline:", game_itr->deadline.utc_seconds);
        }
      }

      /// @abi action
      void offerbet(  const uint64_t game_id, const uint64_t opt_id, const account_name user, const asset& bet) {
        require_auth( user );
        eosio_assert( bet.is_valid(), "invalid bet" );
        eosio_assert( bet.symbol == CORE_SYMBOL, "only core token allowed" );
        eosio_assert( bet.amount > 0, "must bet positive quantity" );
        auto game_itr = games.find(game_id);
        eosio_assert( game_itr != games.end(), "unknown game_id" );
        auto opt_itr = options.find(opt_id);
        eosio_assert( opt_itr != options.end(), "unknown opt_id" );
        eosio_assert( opt_itr->game_id == game_id, "invalid game_id:opt_id pair" );
        eosio_assert( game_itr->deadline.utc_seconds > now(), "game expired");

        action(
            permission_level{ user, N(active) },
            N(eosio.token), N(transfer),
            std::make_tuple(user, _self, bet, std::string(""))
        ).send();

        offers.emplace(_self, [&](auto& new_offer){
          new_offer.id = offers.available_primary_key();
          new_offer.game_id = game_id;
          new_offer.opt_id = opt_id;
          new_offer.owner = user;
          new_offer.bet = bet;
          new_offer.create_time.utc_seconds = now();
        });
      }

      /// @abi action
      void mybets( const account_name user ) {
        require_auth( user );
        auto idx = offers.template get_index<N(owner)>();
        auto owner_itr = idx.find( user );
        for(;owner_itr!=idx.end() && owner_itr->owner==user; owner_itr++) {
          print(" | id:", owner_itr->id, "/game_id:", owner_itr->game_id, "/opt_id:", owner_itr->opt_id, "/bet:", owner_itr->bet, "/status:", owner_itr->status);
        }
      }

      /// @abi action
      void setgameopts( const uint64_t id, const uint64_t game_id, const std::string& opt_name, const account_name creator) {
        require_auth( creator );
        eosio_assert( creator == _self, "only owner can call this action");
        eosio_assert( opt_name.length() > 0, "option name cannot be null");
        auto game_itr = games.find(game_id);
        eosio_assert( game_itr != games.end(), "unknown game_id");

        auto option_itr = options.find(id);
        if (option_itr == options.end()) {
          options.emplace(_self, [&](auto& new_option){
            new_option.id = id;
            new_option.game_id = game_id;
            new_option.name = opt_name;
            new_option.create_time.utc_seconds = now();
          });
        } else {
          options.modify(option_itr, 0, [&](auto& option){
            option.game_id = game_id;
            option.name = opt_name;
          });
        }
      }

      /// @abi action
      void getgameopts( const uint64_t game_id, const account_name user) {
        require_auth( user );
        auto game_itr = games.find(game_id);
        eosio_assert( game_itr!=games.end(), "cannot find game_id");

        auto idx = options.template get_index<N(game_id)>();
        auto opt_itr = idx.find( game_id );
        if (opt_itr != idx.end()) {
          for(; opt_itr!=idx.end() && opt_itr->game_id==game_id; opt_itr++ )  {
            print(" | id:", opt_itr->id, "/game_id:", opt_itr->game_id, "/name:", opt_itr->name);
          }
        } else {
          print("unknown game_id");
        }
      }

      /// @abi action
      void getodds( const uint64_t game_id, const account_name user ) {
        require_auth( user );

        auto opt_idx = options.template get_index<N(game_id)>();
        auto opt_itr = opt_idx.find( game_id );
        eosio_assert(opt_itr!=opt_idx.end(), "unknow game_id");
        std::map<uint64_t, uint64_t> opt_maps;
        for(; opt_itr!=opt_idx.end() && opt_itr->game_id==game_id; opt_itr++) {
          opt_maps[opt_itr->id] = 0LL;
        }

        auto offer_idx = offers.template get_index<N(game_id)>();
        auto offer_itr = offer_idx.find( game_id );
        eosio_assert(offer_itr!=offer_idx.end(), "no user bets");
        asset total;
        uint64_t amount = 0;
        for(; offer_itr!=offer_idx.end() && offer_itr->game_id==game_id ; offer_itr++) {
          amount = offer_itr->bet.amount;
          total.amount += amount;
          opt_maps[offer_itr->opt_id] += amount;
        }

        print("total:", total);
        for(auto map_itr = opt_maps.begin(); map_itr != opt_maps.end(); map_itr++) {
          amount = map_itr->second;
          if (amount == 0) {
            print(" | opt_id:", map_itr->first, "/odds:", amount);
          } else {
            print(" | opt_id:", map_itr->first, "/odds:", ((float)total.amount/(float)amount)*rate);
          }
        }
      }

      /// @abi action
      void lottery( const uint64_t game_id, const uint64_t opt_id, const std::string& name, const account_name creator) {
        require_auth( creator );
        eosio_assert( creator == _self, "only owner can call this action");
        auto game_itr = games.find(game_id);
        eosio_assert( game_itr != games.end(), "unknown game_id" );
        auto opt_itr = options.find(opt_id);
        eosio_assert( opt_itr != options.end(), "unknown opt_id" );
        eosio_assert( opt_itr->game_id == game_id, "invalid game_id:opt_id pair" );
        eosio_assert( name.length()>0, "name can't be null");
        eosio_assert( opt_itr->name == name, "unknown name");
        eosio_assert( game_itr->deadline.utc_seconds < now()+105*60, "not now");

        auto opt_idx = options.template get_index<N(game_id)>();
        auto opt_itr2 = opt_idx.find( game_id );
        std::map<uint64_t, uint64_t> opt_maps;
        for(; opt_itr2!=opt_idx.end() && opt_itr2->game_id==game_id; opt_itr2++) {
          opt_maps[opt_itr2->id] = 0LL;
        }

        auto offer_idx = offers.template get_index<N(game_id)>();
        auto offer_itr = offer_idx.find( game_id );
        uint64_t total = 0;
        uint64_t amount = 0;
        for(; offer_itr!=offer_idx.end() && offer_itr->game_id==game_id ; offer_itr++) {
          amount = offer_itr->bet.amount;
          total += amount;
          opt_maps[offer_itr->opt_id] += amount;
        }
        
        amount = opt_maps[opt_id];
        float odds = (float)total/(float)amount*rate;

        account_name winner;
        asset prize;
        offer_itr = offer_idx.find( game_id );
        for(; offer_itr!=offer_idx.end() && offer_itr->game_id==game_id; offer_itr++) {
          if (offer_itr->opt_id == opt_id) {
            if (offer_itr->status == 0) {
              winner = offer_itr->owner;
              prize = offer_itr->bet;
              prize.amount = offer_itr->bet.amount * odds;

              action(
                  permission_level{ _self, N(active) },
                  N(eosio.token), N(transfer),
                  std::make_tuple(_self, winner, prize, std::string("football lottery from kaiz"))
              ).send();
            }
          }
          auto cur_itr = offers.find(offer_itr->id);
          offers.modify(cur_itr, 0, [&](auto& cur_offer){
            cur_offer.status = 1;
          });
        }
      }

      //@abi table option i64
      struct option {
        uint64_t id;
        uint64_t game_id;
        std::string name;
        eosio::time_point_sec create_time;

        uint64_t primary_key()const { return id; }
        uint64_t get_game_id()const { return game_id; }
        EOSLIB_SERIALIZE( option, (id)(game_id)(name)(create_time));
      };

      typedef eosio::multi_index< N(option), option,
        eosio::indexed_by< N(game_id), eosio::const_mem_fun<option, uint64_t, &option::get_game_id>>
      > option_index;
      option_index options;

      //@abi table game i64
      struct game {
        uint64_t id;
        std::string name;
        account_name owner;
        eosio::time_point_sec deadline;
        eosio::time_point_sec create_time;

        uint64_t primary_key()const { return id; }

        EOSLIB_SERIALIZE( game, (id)(name)(owner)(deadline)(create_time))
      };

      typedef eosio::multi_index< N(game), game> game_index;
      game_index        games;

      //@abi table offer i64
      struct offer {
        uint64_t          id;
        uint64_t          game_id = 0;
        uint64_t          opt_id = 0;
        account_name      owner;
        asset             bet;
        uint32_t          status = 0;
        eosio::time_point_sec create_time;

        uint64_t primary_key()const { return id; }

        uint64_t by_game()const { return game_id; }

        uint64_t by_opt()const { return opt_id; }

        uint64_t by_owner()const { return owner; }

         EOSLIB_SERIALIZE( offer, (id)(game_id)(opt_id)(owner)(bet)(status)(create_time) )
      };

      typedef eosio::multi_index< N(offer), offer,
        indexed_by< N(game_id), const_mem_fun<offer, uint64_t,  &offer::by_game> >,
        indexed_by< N(opt_id), const_mem_fun<offer, uint64_t, &offer::by_opt > >,
        indexed_by< N(owner), const_mem_fun<offer, uint64_t, &offer::by_owner > >
      > offer_index;
      offer_index       offers;
};

EOSIO_ABI( hello, (ver)(setgame)(getgames)(setgameopts)(getgameopts)(offerbet)(mybets)(getodds)(lottery) )
