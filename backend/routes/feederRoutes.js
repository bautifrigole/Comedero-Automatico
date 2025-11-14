const express = require("express");
const router = express.Router();
const { aedes } = require("../services/mqttService"); // Importamos el broker
const { setDuration } = require("../services/configService");

/**
 * Función helper para publicar mensajes MQTT y manejar la respuesta HTTP.
 * @param {object} res - El objeto de respuesta de Express.
 * @param {string} device - El ID del dispositivo (ej. "Feeder-01").
 * @param {string} command - El nombre del comando para logs y respuesta (ej. "START_MOTOR").
 * @param {string} payload - El string que se enviará como payload en el mensaje MQTT.
 * @param {object|null} jsonPayload - Un objeto opcional para el topic JSON.
 */
const publishOrder = (res, device, command, payload, jsonPayload = null) => {
  const message = {
    topic: `${device}/orders`,
    payload: payload,
    qos: 1,
    retain: false,
  };

  aedes.publish(message, (err) => {
    if (err) {
      console.error(`[API] MQTT Error publishing "${command}":`, err);
      return res.status(500).json({ ok: false, error: "Error sending order" });
    }

    // Opcionalmente publicar también en formato JSON
    if (jsonPayload) {
      aedes.publish({
        topic: `${device}/orders/json`,
        payload: JSON.stringify(jsonPayload),
        qos: 1,
        retain: false,
      });
    }

    console.log(
      `[API] Orden MQTT "${command}" sent to the topic ${device}/orders`
    );
    res.status(200).json({ ok: true, message: `Command "${command}" sent` });
  });
};

// Endpoint para iniciar el motor manualmente
router.post("/start-motor", (req, res) => {
  console.log("API: Received request to start motor...");
  const { device = "Feeder-01" } = req.body || {};
  publishOrder(res, device, "START_MOTOR", "START_MOTOR");
});

// Endpoint para detener el motor manualmente
router.post("/stop-motor", (req, res) => {
  console.log("API: Received request to stop motor...");
  const { device = "Feeder-01" } = req.body || {};
  publishOrder(res, device, "STOP_MOTOR", "STOP_MOTOR");
});

router.post("/dispense", (req, res) => {
  console.log("API: Received request to dispense food...");
  const { device = "Feeder-01" } = req.body || {};
  publishOrder(res, device, "DISPENSE_FOOD", "DISPENSE_FOOD");
});

// --- NUEVOS ENDPOINTS PARA LA BOMBA ---

// Endpoint para iniciar la bomba manualmente
router.post("/start-pump", (req, res) => {
  console.log("API: Received request to start pump...");
  const { device = "Feeder-01" } = req.body || {};
  publishOrder(res, device, "START_PUMP", "START_PUMP");
});

// Endpoint para detener la bomba manualmente
router.post("/stop-pump", (req, res) => {
  console.log("API: Received request to stop pump...");
  const { device = "Feeder-01" } = req.body || {};
  publishOrder(res, device, "STOP_PUMP", "STOP_PUMP");
});

// Endpoint para dispensar una porción de agua
router.post("/water", (req, res) => {
  console.log("API: Received request to dispense water...");
  const { device = "Feeder-01" } = req.body || {};
  publishOrder(res, device, "DISPENSE_WATER", "DISPENSE_WATER");
});

// Endpoint genérico para enviar cualquier comando permitido
router.post("/order", (req, res) => {
  const { device = "Feeder-01", command, turns } = req.body;

  if (!command) {
    return res
      .status(400)
      .json({ ok: false, error: "Field 'command' is required" });
  }

  // Se agregaron los comandos de la bomba a la lista de comandos permitidos
  const allowedCommands = [
    "DISPENSE_FOOD",
    "START_MOTOR",
    "STOP_MOTOR",
    "START_PUMP",
    "STOP_PUMP",
    "DISPENSE_WATER",
  ];
  if (!allowedCommands.includes(command)) {
    return res.status(400).json({ ok: false, error: "Invalid command" });
  }

  publishOrder(res, device, command, command, { command, turns });
});

// Endpoint para listar dispositivos conectados
router.get("/devices", (req, res) => {
  const clients = Object.values(aedes.clients).map((client) => ({
    id: client.id,
    connectedAt: new Date(client.conn.connectTime).toISOString(),
  }));
  res.json(clients);
});

/**
 * Ruta: POST /api/set-duration
 * Establece la duración de la ración en segundos.
 * Espera un body: { "duration": 10 }
 */
router.post("/set-duration", (req, res) => {
  const { duration } = req.body;

  if (typeof duration !== 'number' || duration <= 0 || duration > 60) {
    return res.status(400).json({ 
      ok: false, 
      error: "Invalid duration. Must be a number in seconds (e.g., 10), max 60." 
    });
  }

  try {
    const updatedDuration = setDuration(duration);
    
    res.status(200).json({ 
      ok: true, 
      message: `Ration duration set to ${updatedDuration} seconds.` 
    });

  } catch (err) {
     console.error(`[API] Error en set-duration:`, err);
     res.status(500).json({ ok: false, error: "Error updating config" });
  }
});

module.exports = router;
