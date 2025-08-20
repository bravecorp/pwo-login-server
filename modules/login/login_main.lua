local _onReceiveNetworkMessage

function init()
  module:connect("onReceiveNetworkMessage", _onReceiveNetworkMessage, ProtocolCode.GameServerHost)
end

function _onReceiveNetworkMessage(args)
  local msg = args.msg
  local client = args.client
  local clientId = client:getId()

  local instanceName = msg:getString()
  local instanceId = msg:getString()

  g_login.requestCentralAnswer(
    {
      action = "game-server-host",
      instanceName = instanceName,
      instanceId = instanceId
    },
    function(answer)
      local client = g_login.getClient(clientId)
      if not client then
        return
      end

      if not answer.success then
        return
      end

      local host = answer.host
      local port = answer.port
      g_login.sendGameServerHost(client, host, port)
    end
  )
end
