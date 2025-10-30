const express = require("express");
const router = express.Router();
const { getSchedule, setSchedule } = require("../services/scheduleService");

// Obtener los horarios actuales (comida y agua)
router.get("/schedule", (req, res) => {
  const schedule = getSchedule(); // getSchedule() ahora retorna { feeding: [], water: [] }
  res.json({ schedule });
});

// Actualizar el horario (soporta { "feeding": ["HH:MM", ...], "water": ["HH:MM", ...] })
router.post("/set-schedule", (req, res) => {
  // Espera un body en formato JSON que puede contener 'feeding' y/o 'water'
  const schedules = req.body;

  // Validación básica: al menos un array debe estar presente para actualizar
  if (
    !schedules ||
    (!Array.isArray(schedules.feeding) && !Array.isArray(schedules.water))
  ) {
    return res
      .status(400)
      .json({ error: "Body must contain array 'feeding' and/or 'water'." });
  }

  // setSchedule() maneja la actualización de ambos arrays
  const updatedSchedule = setSchedule(schedules);
  res.json({
    message: "Schedule updated succesfully",
    schedule: updatedSchedule,
  });
});

module.exports = router;
