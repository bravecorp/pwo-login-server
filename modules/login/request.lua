local _onRedisMessage

function init()
  g_redis.subscribe("login")
  module:connect("onRedisMessage", _onRedisMessage, "login")
end

function _onRedisMessage(args)
  local message = args.message
  local data = json.decode(message)
  local __answer = data.__answer

  if __answer then
    local __answerId = data.__answerId
    local callback = g_login.answerCallbacks[__answerId]
    if not callback then
      return
    end

    data.__answer = nil
    data.__answerId = nil
    callback(data)
    g_login.answerCallbacks[__answerId] = nil
  end
end
