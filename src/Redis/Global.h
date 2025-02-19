
#ifndef GLOBAL_H
#define GLOBAL_H
#include<iostream>
#include<unordered_map>
#include<sstream>

enum SET_MODEL{
    NONE,NX,XX
};

enum Command{
    SET,
    SETNX,
    SETEX,
    GET,
    SELECT,
    DBSIZE,
    EXISTS,
    DEL,
    RENAME,
    INCR,
    INCRBY,
    INCRBYFLOAT,
    DECR,
    DECRBY,
    MSET,
    MGET,
    STRLEN,
    APPEND,
    KEYS,
    LPUSH,
    RPUSH,
    LPOP,
    RPOP,
    LRANGE,
    HSET,
    HGET,
    HMSET,
    HMGET,
    HDEL,
    HGETALL,
    HKEYS,
    HVALS,
    MULTI,
    EXEC,
    DISCARD,
    COMMAND,
    SADD,
    SREM,
    SMEMBERS,
    SISMEMEBER,
    INVALID_COMMAND
};

static std::unordered_map<std::string,enum Command>commandMaps={
    {"set",SET},
    {"setnx",SETNX},
    {"setex",SETEX},
    {"get",GET},
    {"select",SELECT},
    {"dbsize",DBSIZE},
    {"exists",EXISTS},
    {"del",DEL},
    {"rename",RENAME},
    {"incr",INCR},
    {"incrby",INCRBY},
    {"incrbyfloat",INCRBYFLOAT},
    {"decr",DECR},
    {"decrby",DECRBY},
    {"mset",MSET},
    {"mget",MGET},
    {"hmset",HMSET},
    {"hmget",HMGET},
    {"strlen",STRLEN},
    {"append",APPEND},
    {"keys",KEYS},
    {"lpush",LPUSH},
    {"rpush",RPUSH},
    {"lpop",LPOP},
    {"rpop",RPOP},
    {"lrange",LRANGE},
    {"hset",HSET},
    {"hget",HGET},
    {"hdel",HDEL},
    {"hgetall",HGETALL},
    {"hkeys",HKEYS},
    {"hvals",HVALS},
    {"multi",MULTI},
    {"exec",EXEC},
    {"discard",DISCARD},
    {"command",COMMAND},
    {"sadd",SADD},
    {"srem",SREM},
    {"smembers",SMEMBERS},
    {"sismember",SISMEMEBER}
};

static std::unordered_map<std::string, std::string> cmdDataTypeMaps={
    {"set","STRING"},
    {"setnx","STRING"},
    {"setex","STRING"},
    {"get","STRING"},
    {"select","ALL"},
    {"dbsize","ALL"},
    {"exists","ALL"},
    {"del","ALL"},
    //{"rename",RENAME},
    {"incr","STRING"},
    {"incrby","STRING"},
    //{"incrbyfloat",INCRBYFLOAT},
    {"decr","STRING"},
    {"decrby","STRING"},
    {"mset","STRING"},
    {"mget","STRING"},
    {"hmset","HASH"},
    {"hmget","HASH"},
    {"strlen","STRING"},
    {"append","STRING"},
    {"keys","ALL"},
    {"lpush","LIST"},
    {"rpush","LIST"},
    {"lpop","LIST"},
    {"rpop","LIST"},
    {"lrange","LIST"},
     {"hset","HASH"},
     {"hget","HASH"},
     {"hdel","HASH"},
     {"hgetall","HASH"},
    // {"hkeys",HKEYS},
    // {"hvals",HVALS},
     {"multi","ALL"},
     {"exec","ALL"},
     {"discard","ALL"},
     {"command","ALL"},
     {"sadd","SET"},
    {"srem","SET"},
    {"smembers","SET"},
    {"sismember","SET"}
};






#endif