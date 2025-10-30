const cron = require("node-cron");
const { aedes } = require("./mqttService");

// Para persistencia, esto podría guardarse en un archivo JSON o una base de datos.
let feedingSchedule = ["08:00", "14:00", "20:00"];
let waterSchedule = ["12:00", "18:00"]; // NUEVO: Horario por defecto para el agua

let lastFedTime = null;
let lastWaterTime = null; // NUEVO: Marcador para la última vez que se dispensó agua

/**
 * Función genérica para chequear el horario y publicar una orden MQTT.
 * @param {string[]} schedule - El array de horarios a chequear.
 * @param {string} payload - El payload MQTT a enviar ("DISPENSE_FOOD" o "DISPENSE_WATER").
 */
const checkScheduleAndPublish = (schedule, payload) => {
  const now = new Date();
  const currentTime = `${now.getHours().toString().padStart(2, "0")}:${now
    .getMinutes()
    .toString()
    .padStart(2, "0")}`;

  let isDue = false;
  if (schedule.includes(currentTime)) {
      if (payload === "DISPENSE_FOOD" && lastFedTime !== currentTime) {
          isDue = true;
          lastFedTime = currentTime;
      } else if (payload === "DISPENSE_WATER" && lastWaterTime !== currentTime) {
          isDue = true;
          lastWaterTime = currentTime;
      }
  }

  if (isDue) {
    console.log(`[Scheduler] ¡Time to ${payload.toLowerCase().replace('_', ' ')}! It's ${currentTime}. Sending order...`);
    
    const message = {
      topic: "Feeder-01/orders",
      payload: payload,
      qos: 1,
      retain: false,
    };

    aedes.publish(message, (err) => {
      if (err) {
        console.error(`[Scheduler] Error publishing "${payload}" in MQTT:`, err);
      } else {
        console.log(`[Scheduler] Order "${payload}" sent succesfully.`);
      }
    });
  }
};

const runAllSchedules = () => {
    // Chequear el horario de comida
    checkScheduleAndPublish(feedingSchedule, "DISPENSE_FOOD");
    
    // Chequear el horario de agua
    checkScheduleAndPublish(waterSchedule, "DISPENSE_WATER");
};


const startScheduler = () => {
  // Ahora ejecuta la función `runAllSchedules` cada minuto
  cron.schedule("* * * * *", runAllSchedules);
};

// Retorna un objeto con ambos horarios
const getSchedule = () => ({
    feeding: feedingSchedule,
    water: waterSchedule
});

// Ahora acepta un objeto con horarios de comida y/o agua para actualizar
const setSchedule = (newSchedules) => {
    if (newSchedules.feeding && Array.isArray(newSchedules.feeding)) {
        feedingSchedule = newSchedules.feeding;
        console.log("[Scheduler] Horario de comida actualizado:", feedingSchedule);
    }
    
    if (newSchedules.water && Array.isArray(newSchedules.water)) {
        waterSchedule = newSchedules.water;
        console.log("[Scheduler] Horario de agua actualizado:", waterSchedule);
    }
    
    return getSchedule();
};

module.exports = {
  startScheduler,
  getSchedule,
  setSchedule,
};