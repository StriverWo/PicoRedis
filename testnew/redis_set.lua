request = function()
    local key = "key" .. math.random(1, 100000)
    local value = "value" .. math.random(1, 100000)
    -- 构造 Redis 协议格式的 SET 命令
    return "*3\r\n$3\r\nSET\r\n$" .. #key .. "\r\n" .. key .. "\r\n$" .. #value .. "\r\n" .. value .. "\r\n"
end
