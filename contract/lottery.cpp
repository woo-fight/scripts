#include <eosiolib/asset.hpp>
#include <eosiolib/contract.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/currency.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/multi_index.hpp>
#include <iostream>
#include <string>
#include <time.h>
#include <utility>
#include <vector>

using eosio::action;
using eosio::asset;
using eosio::const_mem_fun;
using eosio::currency;
using eosio::indexed_by;
using eosio::permission_level;
using std::string;
// using eosio::key256;
// using eosio::name;
// using eosio::print;

class lottery : public eosio::contract
{

  public:
    using contract::contract;
    lottery(account_name self)
        : eosio::contract(self), games(_self, _self), bettings(_self, _self) {}
    /** 创建一局游戏,指定本局游戏筹码数量
* @abi action
*/
    void creategame(asset prize_pool, asset betting_value, uint16_t max_palyer)
    {
        require_auth(_self); //必须是我们自己
        innercreate(prize_pool, betting_value, max_palyer);
    }

    /** 玩家加入游戏
* quantity 一次投注多少
* id 表示加入哪局游戏
* @abi action
*/
    void join(account_name name, uint64_t g_id)
    {
        require_auth(name);
        auto curr_game = games.find(g_id);
        eosio_assert(curr_game != games.end(), "the game dose not exist");

        eosio_assert(curr_game->current_index < curr_game->max_palyer &&
                         curr_game->current_index >= 0,
                     "reached the maximum number of player");
        eosio_assert(curr_game != games.end(), "game dose not exist");
        eosio_assert(curr_game->current_index < curr_game->max_palyer,
                     "reached the maximum number of player");
        auto betting_index = bettings.get_index<N(bygid)>();
        auto curr_game_bettings = betting_index.find(g_id);
        while (curr_game_bettings != betting_index.end() &&
               curr_game_bettings->g_id == g_id)
        {
            // eosio_assert(curr_game_bettings->player_name != name,"已经加入游戏");
            eosio::print("currently placed bet:",
                         eosio::name{curr_game_bettings->player_name}, "betting id:",
                         curr_game_bettings->b_id, "\n");
            ++curr_game_bettings;
        }
        // 当前玩家数加1
        games.modify(curr_game, _self,
                     [&](auto &g) { g.current_index = g.current_index + 1; });

        time date = now();

        //具体玩家为scope建表，这里要关注 ram 的使用情况，应该使用的合约开发者的 ram
        // 当前玩家投注数据记录
        auto betting_record_index = betting_table_type(_self, name);
        // _bettings_append(betting_record_index);
        betting_record_index.emplace(_self, [&](auto &b) {
            b.b_id = bettings.available_primary_key();
            b.g_id = g_id;
            b.player_name = name;
            b.bet = curr_game->betting_value;
            b.lucky_number = curr_game->current_index; //暂时定为成玩家加入序号
            b.date = date;
        });
        //当前合约为scope建表
        //总投注记录+1
        // _bettings_append(bettings);
        bettings.emplace(_self, [&](auto &b) {
            b.b_id = bettings.available_primary_key();
            b.g_id = g_id;
            b.player_name = name;
            b.bet = curr_game->betting_value;
            b.lucky_number = curr_game->current_index; //暂时定为成玩家加入序号
            b.date = date;
        });

        action(permission_level{name, N(active)}, N(eosio.token), N(transfer),
               std::make_tuple(name, _self, curr_game->betting_value,
                               std::string("bet")))
            .send();

        if (curr_game->current_index == curr_game->max_palyer)
        {
            eosio::print("ready to open\n");
            inneropen(curr_game->g_id);
        }
        else
        {
            eosio::print("palyer num not enough\n");
        }
    }

    /** 开奖
** @abi action
*/
    void open(uint64_t g_id)
    {
        require_auth(_self);
        inneropen(g_id);
    }

    /**
* 游戏一直未满，管理员主动结束，返回资金给竞猜者
* @abi action
*/
    void stopgame(uint64_t g_id)
    {
        require_auth(_self);
        auto itr = games.find(g_id);
        eosio_assert(itr != games.end(), "the game dose not exist");
        eosio_assert(itr->end != true, "the game is over");

        auto betting_index = bettings.get_index<N(bygid)>();
        auto game_bettings = betting_index.find(g_id);
        while (game_bettings != betting_index.end() &&
               game_bettings->g_id == g_id)
        {
            //返还用户金额
            action(permission_level{_self, N(active)}, N(eosio.token), N(transfer),
                   std::make_tuple(_self, game_bettings->player_name,
                                   game_bettings->bet, string("")))
                .send();
            ++game_bettings;
        }
        games.modify(itr, _self, [&](auto &g) {
            g.randseed = 0;
            g.end = true;
        });
    }

    /**支付失败的情况下从改局游戏中移除,这种情况不会存在
* @abi action
*/
    void removebetting(uint64_t g_id, uint64_t b_id)
    {
        auto itr = games.find(g_id);
        eosio_assert(itr != games.end(), "the game dose not exist");

        auto betting_index = bettings.get_index<N(bygid)>();
        auto game_bettings = betting_index.find(g_id);
        while (game_bettings != betting_index.end() &&
               game_bettings->g_id == g_id)
        {
            auto betting = bettings.find(game_bettings->b_id);
            if (betting->b_id == b_id)
            {
                eosio::print("cancel betting: ", b_id,
                             eosio::name{betting->player_name});
                bettings.erase(betting);
                games.modify(itr, _self,
                             [&](auto &g) { g.current_index = g.current_index - 1; });
                break;
            }
            eosio::print("currently placed bet:", eosio::name{betting->player_name},
                         "betting id:", game_bettings->b_id);
            ++game_bettings;
        }
    }

  private:
    struct basegame
    {
        uint64_t g_id;
        uint64_t randseed;
        uint16_t end = false; //是否已经开奖
        time date = now();    //开始游戏时间
        auto primary_key() const { return g_id; }

        EOSLIB_SERIALIZE(basegame, (g_id)(randseed)(end));
    };

    ///@abi table lotterygame i64
    struct lotterygame : public basegame
    {
        uint16_t current_index; //当前参与玩家序号
        uint16_t max_palyer;    //本局玩家人数
        asset prize_pool;       //奖金池
        asset betting_value;    //每个投注金额固定
        EOSLIB_SERIALIZE(lotterygame, (g_id)(randseed)(end)(date)(current_index)(
                                          max_palyer)(prize_pool)(betting_value));
    };

    typedef eosio::multi_index<N(lotterygame), lotterygame> game_index;
    ///@abi table betting i64
    struct betting
    {
        uint64_t b_id;            //主键序号
        uint64_t g_id;            //游戏 id
        account_name player_name; //玩家账户
        asset bet;                //投注额度
        uint64_t lucky_number;    //投注号码
        time date = now();        //投注时间
        auto primary_key() const { return b_id; }
        uint64_t game_id() const { return g_id; }
        // account_name player_name() const {return player_name;}
        EOSLIB_SERIALIZE(betting,
                         (b_id)(g_id)(player_name)(bet)(lucky_number)(date));
    };

    typedef eosio::multi_index<
        N(betting), betting,
        indexed_by<N(bygid), const_mem_fun<betting, uint64_t, &betting::game_id>>>
        betting_table_type;
    void inneropen(uint64_t g_id)
    {
        eosio::print("************* inneropen", "\n");
        auto itr = games.find(g_id);
        eosio_assert(itr != games.end(), "the game dose not exist");
        eosio_assert(itr->current_index == itr->max_palyer,
                     "wrong number of players cannot start the game");
        game_rule(g_id);
    }
    /** 创建一局游戏,指定本局游戏筹码数量
* @abi action
*/
    void innercreate(const asset &prize_pool, const asset &betting_value,
                     uint16_t max_palyer)
    {
        // eosio_assert(max_palyer < 100 && max_palyer >= 0,
        //"number of players  beyond the limit(100)");
        eosio::print("innercreate: prize_pool amount:", prize_pool.amount,
                     " betting_value:", betting_value.amount, " max_palyer:",
                     (uint64_t)max_palyer, "\n");

        games.emplace(_self, [&](auto &g) {
            g.g_id = games.available_primary_key();
            g.current_index = 0;
            g.prize_pool = prize_pool;
            g.max_palyer = max_palyer;
            g.betting_value = betting_value;
        });
        auto curr_game = games.find(0);
        eosio_assert(curr_game != games.end(), "the game dose not exist");
        eosio::print("curr_game.g_id:", curr_game->g_id, "\n");
    }
    void game_rule(uint64_t g_id)
    {
        auto game = games.find(g_id);
        eosio_assert(game != games.end(), "the game dose not exist");
        auto betting_index = bettings.get_index<N(bygid)>();
        auto curr_game_bettings = betting_index.find(g_id);
        eosio::print("************** game_rule", "\n");
        // 随机出获奖号码，这里有隐患
        time date = now();
        // srand(date);
        // auto lucky_number = rand() % 100 + 1;
        checksum256 lucky_key;
        sha256((char *)&date, sizeof(time), &lucky_key);
        // eosio::print("lucky_key:", (char *)&date, sizeof(time), &lucky_key);
        auto lucky_number =
            (lucky_key.hash[0] + lucky_key.hash[1] + lucky_key.hash[2]) %
                game->max_palyer +
            1;
        eosio::print("the lucky number:", lucky_number, "\n");
        while (curr_game_bettings != betting_index.end() &&
               curr_game_bettings->g_id == g_id)
        {
            auto lucky_betting = bettings.find(curr_game_bettings->b_id);

            if (lucky_betting->lucky_number == lucky_number)
            {
                eosio::print("the winner:", eosio::name{lucky_betting->player_name},
                             "the winning betting id:", lucky_betting->b_id, "\n");
                //发奖励
                action(permission_level{_self, N(active)}, N(eosio.token), N(transfer),
                       std::make_tuple(_self, lucky_betting->player_name,
                                       game->prize_pool, std::string("winner")))
                    .send();
                break;
            }
            else
            {
                eosio::print("unlucky!!! placed bet:",
                             eosio::name{lucky_betting->player_name}, "betting id:",
                             lucky_betting->b_id, "\n");
            }
            ++curr_game_bettings;
        }
        //当前轮游戏结束
        games.modify(game, _self, [&](auto &g) {
            g.randseed = 0;
            g.end = true;
        });

        // 开始新一轮的游戏
        // creategame(core_from_string("100.0000"), 100);
        innercreate(game->prize_pool, game->betting_value, game->max_palyer);
        // creategame(eosio::chain::asset::from_string("100.0000" " "
        // CORE_SYMBOL_NAME), 100);
    }

    ///检测是否为我们的货币
    void check_my_asset(const asset &quantity, const asset &game_pay)
    {
        /*需不需要精度和符号都相等？此处仅仅符号相等 */
        eosio::print("quantity:", quantity.symbol, " game_pay:", game_pay.symbol,
                     "\n");
        eosio_assert(quantity.symbol == game_pay.symbol, "bad currency type!");
    }

    game_index games;
    betting_table_type bettings;
};
EOSIO_ABI(lottery, (creategame)(join)(open)(removebetting)(stopgame))