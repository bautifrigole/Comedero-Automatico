// backend/index.js

const express = require("express");
const http = require("http");
const net = require("net");
const aedes = require("aedes")();
const ws = require("websocket-stream");

const app = express();
const apiPort = 3001; // Puerto para la API que usa el frontend

// 1. Creamos un servidor HTTP usando Express.
// Este servidor se encargará de las rutas de la API.
const httpServer = http.createServer(app);

// 2. Configuramos el Broker MQTT (Aedes)
const mqttWSPort = 8888; // Puerto WebSockets para frontend
const mqttTCPPort = 1883; // Puerto TCP para ESP32

// WebSocket server para frontend
ws.createServer({ server: httpServer }, aedes.handle);

httpServer.listen(mqttWSPort, function () {
  console.log(
    `🚀 Broker MQTT (sobre WebSockets) escuchando en el puerto ${mqttWSPort}`
  );
});

// TCP server para ESP32
const tcpServer = net.createServer(aedes.handle);
tcpServer.listen(mqttTCPPort, function () {
  console.log(
    `🚀 Broker MQTT (TCP) escuchando en el puerto ${mqttTCPPort} para ESP32`
  );
});

// --- Lógica del Broker (para depuración) ---
aedes.on("client", function (client) {
  console.log(`Cliente conectado: ${client.id}`);
});

aedes.on("publish", function (packet, client) {
  if (client) {
    console.log(`Mensaje de ${client.id}:`, packet.payload.toString());
  }
});

// 3. Configuramos la API REST que usará Next.js
app.use(express.json());

// Endpoint para que el frontend ordene dispensar comida
app.post("/api/dispensar", (req, res) => {
  console.log("API: Recibida petición para dispensar...");
  const { device = "comedero/01" } = req.body;

  const message = {
    topic: `${device}/ordenes`,
    payload: "DISPENSAR_COMIDA",
    qos: 1,
    retain: false,
  };

  // El broker (aedes) publica el mensaje
  aedes.publish(message, (err) => {
    if (err) {
      console.error("MQTT Error:", err);
      return res.status(500).send("Error al enviar la orden");
    }
    console.log(
      `MQTT: Orden "DISPENSAR_COMIDA" enviada al topic ${device}/ordenes`
    );
    res.status(200).send("Orden de dispensar enviada");
  });
});

// Endpoint para iniciar el motor manualmente
app.post("/api/iniciar-motor", (req, res) => {
  console.log("API: Recibida petición para iniciar motor...");
  const { device = "comedero/01" } = req.body;

  const message = {
    topic: `${device}/ordenes`,
    payload: "INICIAR_MOTOR",
    qos: 1,
    retain: false,
  };

  aedes.publish(message, (err) => {
    if (err) {
      console.error("MQTT Error:", err);
      return res.status(500).send("Error al enviar la orden");
    }
    console.log(
      `MQTT: Orden "INICIAR_MOTOR" enviada al topic ${device}/ordenes`
    );
    res.status(200).send("Motor iniciado");
  });
});

// Endpoint para detener el motor manualmente
app.post("/api/detener-motor", (req, res) => {
  console.log("API: Recibida petición para detener motor...");
  const { device = "comedero/01" } = req.body;

  const message = {
    topic: `${device}/ordenes`,
    payload: "DETENER_MOTOR",
    qos: 1,
    retain: false,
  };

  aedes.publish(message, (err) => {
    if (err) {
      console.error("MQTT Error:", err);
      return res.status(500).send("Error al enviar la orden");
    }
    console.log(
      `MQTT: Orden "DETENER_MOTOR" enviada al topic ${device}/ordenes`
    );
    res.status(200).send("Motor detenido");
  });
});

// Endpoint para listar dispositivos conectados
app.get("/api/dispositivos", (req, res) => {
  const clients = [];
  aedes.clients.forEach((client, clientId) => {
    if (clientId.startsWith("ESP32Client-")) {
      clients.push({
        id: clientId,
        connected: client.connected,
        lastSeen: new Date(),
      });
    }
  });
  res.json(clients);
});

// 4. Iniciamos el servidor de la API
app.listen(apiPort, () => {
  console.log(`📡 API REST escuchando en http://localhost:${apiPort}`);
});
