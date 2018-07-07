# eos-football-lottery
EOS足彩DAPP示例，仅供学习参考，请勿用于商业用途。
因UTXO模式限制不能用于高并发正式场景，

##### 设置游戏：
````bash
cleos push action kaiz setgame '["1", "FRA vs URU", "kaiz", "1530885600"]' -p kaiz
````
注意最后一个参数为时间戳，EOS暂时没有字符串转换时间的函数，以及不能获取主机所在时区（如果有请帮忙指出，谢谢~），只能在Shell中获取unix_timestamp之后作为参数传入

##### 查看游戏：
````bash
cleos push action kaiz getgames '["kaiz"]' -p kaiz
````

##### 设置选项：
````bash
cleos push action kaiz setgameopts '["1001", "1", "FRA", "kaiz"]' -p kaiz
cleos push action kaiz setgameopts '["1002", "1", "URU", "kaiz"]' -p kaiz
````

##### 查看选项：
````bash
cleos push action kaiz getgameopts '["1", "kaiz"]' -p kaiz
````

##### 下注：
````bash
cleos push action kaiz offerbet '["1", "1002", "alice", "10.0000 EOS"]' -p alice
cleos push action kaiz offerbet '["1", "1001", "bob", "10.0000 EOS"]' -p bob
````
注意，在游戏设置的时间戳之后是不能再下注的，另外交易需要设置账号对应permission

##### 查看本人投注：
````bash
cleos push action kaiz mybets '["alice"]' -p alice
cleos push action kaiz mybets '["bob"]' -p bob
````

##### 查看游戏赔率：
````bash
cleos push action kaiz getodds '["1", "kaiz"]' -p kaiz
````

##### 开奖：
````bash
cleos push action kaiz lottery '["1", "1001", "FRA", "kaiz"]' -p kaiz
````
注意，在deadline时间过后才能开奖，另外开奖、设置游戏、设置选项都只能dapp的owner才能调用