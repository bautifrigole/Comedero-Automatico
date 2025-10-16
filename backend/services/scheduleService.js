const cron = require("node-cron");
const { aedes } = require("./mqttService");

// Para persistencia, esto podría guardarse en un archivo JSON o una base de datos.
let feedingSchedule = ["08:00", "14:00", "20:00"];

let lastFedTime = null;

const checkScheduleAndFeed = () => {
  const now = new Date();
  const currentTime = `${now.getHours().toString().padStart(2, "0")}:${now
    .getMinutes()
    .toString()
    .padStart(2, "0")}`;

  // Si la hora actual está en el array de horarios Y no hemos alimentado en este minuto
  if (feedingSchedule.includes(currentTime) && lastFedTime !== currentTime) {
    console.log(`[Scheduler] ¡Time to feed! It's ${currentTime}. Sending order...`);
    
    lastFedTime = currentTime; // Marcamos que ya hemos alimentado en este minuto

    const message = {
      topic: "Feeder-01/orders",
      payload: "DISPENSE_FOOD",
      qos: 1,
      retain: false,
    };

    aedes.publish(message, (err) => {
      if (err) {
        console.error("[Scheduler] Error publishing in MQTT:", err);
      } else {
        console.log(`[Scheduler] Order "DISPENSE_FOOD" sent succesfully.`);
      }
    });
  }
};

const startScheduler = () => {
  // Ejecuta la función `checkScheduleAndFeed` cada minuto ('* * * * *')
  cron.schedule("* * * * *", checkScheduleAndFeed);
};

const getSchedule = () => feedingSchedule;

const setSchedule = (newTimes) => {
  feedingSchedule = newTimes;
  console.log("[Scheduler] Horario actualizado:", feedingSchedule);
  return feedingSchedule;
};

module.exports = {
  startScheduler,
  getSchedule,
  setSchedule,
};