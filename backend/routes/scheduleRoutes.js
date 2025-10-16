const express = require("express");
const router = express.Router();
const { getSchedule, setSchedule } = require("../services/scheduleService");

// Obtener el horario actual
router.get("/schedule", (req, res) => {
  const schedule = getSchedule();
  res.json({ schedule });
});

// Actualizar el horario
// Espera un body en formato JSON: { "times": ["07:30", "15:00", "21:30"] }
router.post("/set-schedule", (req, res) => {
  const { times } = req.body;

  if (!Array.isArray(times)) {
    return res.status(400).json({ error: "Body must contain array 'times'." });
  }

  // Aquí podrías agregar una validación más estricta del formato HH:MM
  const updatedSchedule = setSchedule(times);
  res.json({ message: "Schedule updated succesfully", schedule: updatedSchedule });
});

module.exports = router;