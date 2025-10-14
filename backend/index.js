// backend/index.js

const express = require("express");
const http = require("http");
const net = require("net");
const aedes = require("aedes")();
const ws = require("websocket-stream");
const cors = require("cors");

const app = express();
const apiPort = 3001; // Port for the API used by the frontend

// 1. Create an HTTP server using Express.
// This server will handle API routes.
const httpServer = http.createServer(app);

// Enable CORS for all routes
app.use(cors());

// 2. Configure the MQTT Broker (Aedes)
const mqttWSPort = 8888; // WebSocket port for frontend
const mqttTCPPort = 1883; // TCP port for ESP32

// WebSocket server for frontend
ws.createServer({ server: httpServer }, aedes.handle);

httpServer.listen(mqttWSPort, function () {
  console.log(
    `🚀 MQTT Broker (over WebSockets) listening on port ${mqttWSPort}`
  );
});

// TCP server for ESP32
const tcpServer = net.createServer(aedes.handle);
tcpServer.listen(mqttTCPPort, function () {
  console.log(
    `🚀 MQTT Broker (TCP) listening on port ${mqttTCPPort} for ESP32`
  );
});

// --- Broker Logic (for debugging) ---
aedes.on("client", function (client) {
  console.log(`Client connected: ${client.id}`);
});

aedes.on("publish", function (packet, client) {
  if (client) {
    console.log(`Message from ${client.id}:`, packet.payload.toString());
  }
});

// 3. Configure the REST API that Next.js will use
app.use(express.json());

// Endpoint for the frontend to order food dispensing
app.post("/api/dispense", (req, res) => {
  console.log("API: Received request to dispense...");
  const { device = "Feeder-01" } = req.body || {};

  const message = {
    topic: `${device}/orders`,
    payload: "DISPENSE_FOOD",
    qos: 1,
    retain: false,
  };

  // The broker (aedes) publishes the message
  aedes.publish(message, (err) => {
    if (err) {
      console.error("MQTT Error:", err);
      return res.status(500).send("Error sending order");
    }
    console.log(`MQTT: Order "DISPENSE_FOOD" sent to topic ${device}/orders`);
    res.status(200).send("Dispense order sent");
  });
});

// Endpoint to start the motor manually
app.post("/api/start-motor", (req, res) => {
  console.log("API: Received request to start motor...");
  const { device = "Feeder-01" } = req.body || {};

  const message = {
    topic: `${device}/orders`,
    payload: "START_MOTOR",
    qos: 1,
    retain: false,
  };

  aedes.publish(message, (err) => {
    if (err) {
      console.error("MQTT Error:", err);
      return res.status(500).send("Error sending order");
    }
    console.log(`MQTT: Order "START_MOTOR" sent to topic ${device}/orders`);
    res.status(200).send("Motor started");
  });
});

// Endpoint to stop the motor manually
app.post("/api/stop-motor", (req, res) => {
  console.log("API: Received request to stop motor...");
  const { device = "Feeder-01" } = req.body || {};

  const message = {
    topic: `${device}/orders`,
    payload: "STOP_MOTOR",
    qos: 1,
    retain: false,
  };

  aedes.publish(message, (err) => {
    if (err) {
      console.error("MQTT Error:", err);
      return res.status(500).send("Error sending order");
    }
    console.log(`MQTT: Order "STOP_MOTOR" sent to topic ${device}/orders`);
    res.status(200).send("Motor stopped");
  });
});

// Generic endpoint to send orders (compatible and extensible)
// Expected body: { device?: string, command: string, turns?: number }
app.post("/api/order", (req, res) => {
  console.log("API: /api/order received...");
  const body = req.body || {};
  const device = body.device || req.query.device || "Feeder-01";
  const command = body.command || req.query.command;
  const turns = body.turns || req.query.turns || null;

  if (!command) {
    return res.status(400).json({ ok: false, error: "command is required" });
  }

  const allowed = ["DISPENSE_FOOD", "START_MOTOR", "STOP_MOTOR"];
  if (!allowed.includes(command)) {
    return res.status(400).json({ ok: false, error: "invalid command" });
  }

  // Publish simple payload for compatibility with current firmware
  const simpleMessage = {
    topic: `${device}/orders`,
    payload: command,
    qos: 1,
    retain: false,
  };

  // Also publish a JSON with more details in case the firmware supports it
  const jsonPayload = JSON.stringify({ command, turns });
  const jsonMessage = {
    topic: `${device}/orders/json`,
    payload: jsonPayload,
    qos: 1,
    retain: false,
  };

  aedes.publish(simpleMessage, (err) => {
    if (err) {
      console.error("MQTT Error (simple):", err);
      return res.status(500).json({ ok: false, error: "mqtt publish failed" });
    }

    // Try to publish the JSON as well (not critical)
    aedes.publish(jsonMessage, (err2) => {
      if (err2) {
        console.warn("MQTT Warning (json publish):", err2);
      }
      console.log(`MQTT: Order "${command}" sent to ${device}`);
      return res.json({ ok: true, device, command, turns });
    });
  });
});

// Endpoint to list connected devices
app.get("/api/devices", (req, res) => {
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

// 4. Start the API server
app.listen(apiPort, () => {
  console.log(`📡 REST API listening at http://localhost:${apiPort}`);
});
