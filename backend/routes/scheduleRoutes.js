const express = require("express");
const router = express.Router();
const { getSchedule, setSchedule, getDuration } = require("../services/configService");

router.get("/schedule", (req, res) => {
  const schedule = getSchedule();
  const duration = getDuration();
  res.json({
    schedule: {feeding: schedule },
    duration: duration
  });
});

router.post("/set-schedule", (req, res) => {
  const schedule = req.body.feeding;

  if (!schedule || !Array.isArray(schedule)) {
    return res
      .status(400)
      .json({ error: "Body must contain a 'feeding' array." });
  }

  const updatedSchedule = setSchedule(schedule);
  
  res.json({
    message: "Schedule updated succesfully",
    schedule: {feeding: updatedSchedule},
  });
});

module.exports = router;