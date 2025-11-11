const net = require("net");
const aedes = require("aedes")();
const ws = require("websocket-stream");
const mqtt = require("mqtt"); // Usaremos el cliente MQTT estándar de Node.js

const mqttTCPPort = 1883; // Puerto TCP para el ESP32
// Tópico al que se suscribirá el backend para recibir notificaciones del dispositivo
const deviceLevelTopic = "Feeder-01/level";

// Variable global para el cliente MQTT del backend
let backendMqttClient = null;

aedes.on("client", function (client) {
  console.log(`[MQTT] Client connected: ${client.id}`);
});

// Aumentar la lógica de 'publish' para interceptar mensajes entrantes (del ESP)
aedes.on("publish", function (packet, client) {
  const payload = packet.payload.toString();
  const topic = packet.topic;

  // Los mensajes publicados por el broker (backendMqttClient) no tienen 'client' asociado.
  if (client) {
    // Mensaje saliente del dispositivo
    console.log(`[MQTT] Message from ${client.id} on ${topic}:`, payload);
  } else {
    // Mensaje entrante al backend (por ejemplo, el mensaje de nivel del ESP)
    console.log(`[MQTT] Received message on topic ${topic}:`, payload);
  }
  
  // 1. Lógica para el ESTADO GENERAL del comedero
  if (topic.endsWith("/status")) {
    // Ejemplo: Notificar si una ración de comida/agua ha terminado
    if (payload === "RATION_COMPLETE") {
      console.log(`✅ DISPENSACIÓN DE COMIDA FINALIZADA para ${topic.split('/')[0]}`);
    } else if (payload === "WATER_RATION_COMPLETE") {
      console.log(`✅ DISPENSACIÓN DE AGUA FINALIZADA para ${topic.split('/')[0]}`);
    }
  } 
  
  // 2. Lógica para el NIVEL DE COMIDA
  else if (topic === deviceLevelTopic) {
    if (payload === "FOOD_LEVEL_LOW") {
      console.log(`⚠️ ALERTA DE NIVEL BAJO: ¡El comedero ${topic.split('/')[0]} se está quedando sin comida!`);
      // Aquí podrías añadir lógica para enviar notificaciones push o emails
    } else if (payload === "FOOD_LEVEL_OK") {
      console.log(`🟢 Nivel de comida OK para ${topic.split('/')[0]}`);
    }
  }
});


/**
 * Inicializa el broker Aedes y conecta el backend como un cliente MQTT para suscripciones.
 * @param {object} httpServer - Servidor HTTP para WebSocket.
 */
const startBroker = (httpServer) => {
  // 1. Servidor WebSocket para el frontend (se acopla al servidor HTTP principal)
  ws.createServer({ server: httpServer }, aedes.handle);

  // 2. Servidor TCP para los dispositivos IoT (ESP32)
  const tcpServer = net.createServer(aedes.handle);
  tcpServer.listen(mqttTCPPort, function () {
    console.log(
      `🚀 Broker MQTT (TCP) listening on port ${mqttTCPPort}`
    );
    
    // CORRECCIÓN: Usar el cliente 'mqtt' para conectarse al broker TCP (localhost:1883)
    backendMqttClient = mqtt.connect(`mqtt://localhost:${mqttTCPPort}`, {
        clientId: "backend-feeder-api",
        clean: true,
        connectTimeout: 4000,
        reconnectPeriod: 1000,
    });
    
    backendMqttClient.on('connect', () => {
        console.log("[MQTT] Backend client connected as: backend-feeder-api");
        
        // Suscribirse al tópico de nivel de comida
        backendMqttClient.subscribe(deviceLevelTopic, { qos: 1 }, (err) => {
            if (err) {
                console.error(`[MQTT] Error subscribing to ${deviceLevelTopic}:`, err);
            } else {
                console.log(`[MQTT] Successfully subscribed to: ${deviceLevelTopic}`);
            }
        });
    });

    backendMqttClient.on('error', (error) => {
        console.error("[MQTT] Backend client error:", error);
    });

    // Los mensajes recibidos por el backend ahora se manejarán a través del evento 'publish' de Aedes,
    // que intercepta todo el tráfico.
  });
};

module.exports = { aedes, startBroker };