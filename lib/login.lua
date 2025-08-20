g_login = {
  answerIdGenerator = 0,
  answerCallbacks = {}
}

function g_login.requestCentralAnswer(data, callback)
  g_login.answerIdGenerator = g_login.answerIdGenerator + 1
  data.__answer = true
  data.__answerId = g_login.answerIdGenerator

  g_login.answerCallbacks[g_login.answerIdGenerator] = callback

  if not g_redis.publish("central_login", json.encode(data)) then
    g_login.answerCallbacks[g_login.answerIdGenerator] = nil
  end
end

function g_login.sendGameServerHost(client, host, port)
  local msg = OutputMessage()
  msg:addU8(ProtocolCode.GameServerHost)
  msg:addString(host)
  msg:addU16(port)
  client:send(msg)
end
