const fs = require('fs');
const path = require('path');
const { aedes } = require("./mqttService"); // Importamos el broker

// Ruta al archivo de configuración
const CONFIG_FILE_PATH = path.join(__dirname, '..', 'config.json');

// Configuración por defecto si el archivo no existe
const DEFAULT_CONFIG = {
  deviceName: "Feeder-01",
  feedingSchedule: ["08:00", "14:00", "20:00"],
  rationDuration: 10 // Duración en segundos
};

// Variable en memoria que guarda la configuración
let config = { ...DEFAULT_CONFIG };

/**
 * Guarda el estado actual de 'config' en el archivo config.json
 */
const saveConfig = () => {
  try {
    fs.writeFileSync(CONFIG_FILE_PATH, JSON.stringify(config, null, 2));
    console.log('[Config] Configuración guardada en config.json');
  } catch (err) {
    console.error('[Config] ERROR: No se pudo guardar config.json', err);
  }
};

/**
 * Carga la configuración desde config.json al arrancar
 */
const loadConfig = () => {
  try {
    if (fs.existsSync(CONFIG_FILE_PATH)) {
      const data = fs.readFileSync(CONFIG_FILE_PATH, 'utf-8');
      config = JSON.parse(data);
      console.log('[Config] Configuración cargada desde config.json');
    } else {
      console.log('[Config] No se encontró config.json, creando uno nuevo.');
      config = { ...DEFAULT_CONFIG };
      saveConfig();
    }
  } catch (err) {
    console.error('[Config] ERROR: No se pudo leer config.json. Usando defaults.', err);
    config = { ...DEFAULT_CONFIG };
  }
};

/**
 * Helper para publicar un mensaje de configuración al dispositivo
 * @param {string} topic - Tópico MQTT (ej. '/config/schedule')
 * @param {string} payload - El payload a enviar
 */
const publishConfigToDevice = (topic, payload) => {
  const fullTopic = `${config.deviceName}/${topic}`;
  const message = {
    topic: fullTopic,
    payload: String(payload),
    qos: 1,
    retain: true // Retener siempre los mensajes de configuración
  };

  aedes.publish(message, (err) => {
    if (err) {
      console.error(`[Config] Error publicando en ${fullTopic}:`, err);
    } else {
      console.log(`[Config] Configuración publicada en ${fullTopic}: ${payload}`);
    }
  });
};

// --- Funciones Públicas ---

/**
 * Inicia el servicio: carga la config y publica los valores retenidos
 */
const initConfigService = () => {
  loadConfig();
  // Publicar todas las configuraciones al broker al iniciar
  // para que el ESP las reciba si se (re)conecta.
  console.log('[Config] Publicando configuración inicial al broker (retain=true).');
  publishConfigToDevice('config/schedule', config.feedingSchedule.join(','));
  publishConfigToDevice('config/duration', config.rationDuration);
};

// --- Funciones para Horarios ---

const getSchedule = () => {
  loadConfig();
  return config.feedingSchedule;
};

const setSchedule = (newTimes) => {
  if (!Array.isArray(newTimes)) {
    console.error("[Config] setSchedule recibió un formato inválido.");
    return config.feedingSchedule;
  }
  
  config.feedingSchedule = newTimes;
  saveConfig(); // Guardar en el archivo JSON
  
  // Publicar el cambio en MQTT
  publishConfigToDevice('config/schedule', config.feedingSchedule.join(','));
  
  return config.feedingSchedule;
};

// --- Funciones para Duración (NUEVO) ---

const getDuration = () => {
  loadConfig();
  return config.rationDuration;
};

const setDuration = (durationInSeconds) => {
  const durationNum = Number(durationInSeconds);
  if (isNaN(durationNum) || durationNum <= 0 || durationNum > 60) {
    console.error("[Config] setDuration recibió una duración inválida.");
    return config.rationDuration;
  }
  
  config.rationDuration = durationNum;
  saveConfig(); // Guardar en el archivo JSON
  
  // Publicar el cambio en MQTT
  publishConfigToDevice('config/duration', config.rationDuration);
  
  return config.rationDuration;
};

module.exports = {
  initConfigService, // Reemplaza al antiguo startScheduler
  getSchedule,
  setSchedule,
  getDuration,
  setDuration,
  getConfig: () => config // Helper para otras partes si es necesario
};