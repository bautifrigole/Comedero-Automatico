const net = require("net");
const aedes = require("aedes")();
const ws = require("websocket-stream");

const mqttTCPPort = 1883; // Puerto TCP para el ESP32

aedes.on("client", function (client) {
  console.log(`[MQTT] Client connected: ${client.id}`);
});

aedes.on("publish", function (packet, client) {
  if (client) {
    console.log(
      `[MQTT] Message from ${client.id}:`,
      packet.payload.toString()
    );
  }
});

// Función para iniciar el broker en sus diferentes transportes
const startBroker = (httpServer) => {
  // 1. Servidor WebSocket para el frontend (se acopla al servidor HTTP principal)
  ws.createServer({ server: httpServer }, aedes.handle);

  // 2. Servidor TCP para los dispositivos IoT (ESP32)
  const tcpServer = net.createServer(aedes.handle);
  tcpServer.listen(mqttTCPPort, function () {
    console.log(
      `🚀 Broker MQTT (TCP) listening on port ${mqttTCPPort}`
    );
  });
};

module.exports = { aedes, startBroker };