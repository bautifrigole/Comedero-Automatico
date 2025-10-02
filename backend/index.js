// backend/index.js

const express = require("express");
const http = require("http");
const aedes = require("aedes")();
const ws = require("websocket-stream");

const app = express();
const apiPort = 3001; // Puerto para la API que usa el frontend

// 1. Creamos un servidor HTTP usando Express.
// Este servidor se encargará de las rutas de la API.
const httpServer = http.createServer(app);

// 2. Configuramos el Broker MQTT (Aedes)
const mqttPort = 8888; // Puerto para que el ESP32 se conecte vía WebSockets

ws.createServer({ server: httpServer }, aedes.handle);

httpServer.listen(mqttPort, function () {
  console.log(
    `🚀 Broker MQTT (sobre WebSockets) escuchando en el puerto ${mqttPort}`
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

  const message = {
    topic: "comedero/01/ordenes",
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
    console.log('MQTT: Orden "DISPENSAR_COMIDA" enviada al topic.');
    res.status(200).send("Orden de dispensar enviada");
  });
});

// 4. Iniciamos el servidor de la API
app.listen(apiPort, () => {
  console.log(`📡 API REST escuchando en http://localhost:${apiPort}`);
});
