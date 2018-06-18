# mkdir -p logs
# nohup nodeos --data-dir ./data-dir --config-dir ./config-dir  > ./logs/eos.log 2>&1 &
# echo $! > eos.pid
# nohup keosd -d ./data-dir --http-server-address localhost:8900 >./logs/wallet.log 2>&1 &
# echo $! > wallet.pid

# cleos wallet create
cleos set contract eosio $EOS_BUILD_DIR/contracts/eosio.bios/ -p eosio

# eosio.token
cleos create account eosio eosio.token EOS6QahzfDoDvdXGTjV8YWxZ6vEYHqxen9v16U1aRZj7kWiPpmb4E EOS6QahzfDoDvdXGTjV8YWxZ6vEYHqxen9v16U1aRZj7kWiPpmb4E
cleos create account eosio eosio.msig EOS6QahzfDoDvdXGTjV8YWxZ6vEYHqxen9v16U1aRZj7kWiPpmb4E EOS6QahzfDoDvdXGTjV8YWxZ6vEYHqxen9v16U1aRZj7kWiPpmb4E
cleos wallet import 5KTdTrTr1pYzLQjo8hSUmtzShEtHaonmVfHF7mAdv3azpxhE6Xr

cleos set contract eosio.token $EOS_BUILD_DIR/contracts/eosio.token -p eosio.token
cleos set contract eosio.msig $EOS_BUILD_DIR/contracts/eosio.token -p eosio.msig

cleos push action eosio.token create '["eosio", "1000000000.0000 SYS", 0, 0, 0]' -p eosio.token
cleos push action eosio.token issue '["eosio", "1000000000.0000 SYS", "xxxxx"]' -p eosio

# eosio.system
cleos set contract eosio $EOS_BUILD_DIR/contracts/eosio.system -p eosio

cleos system newaccount eosio frank1111111 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV --stake-net "1.0000 SYS" --stake-cpu "1.0000 SYS" --buy-ram "1.0000 SYS"
cleos system newaccount eosio producer1111 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV --stake-net "1.0000 SYS" --stake-cpu "1.0000 SYS" --buy-ram "1.0000 SYS"

cleos system regproducer frank1111111 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV eosio.sg
cleos system regproducer producer1111 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV eosio.sg

cleos transfer eosio frank1111111 "800000000.0000 SYS"
cleos transfer frank1111111 producer1111 "10000.0000 SYS"

cleos system delegatebw frank1111111 frank1111111 "70000000.0000 SYS" "40000000.0000 SYS" --transfer
cleos system voteproducer prods frank1111111 frank1111111 producer1111
