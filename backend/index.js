const express = require("express");
const http = require("http");
const cors = require("cors");
const morgan = require("morgan");
const { startBroker } = require("./services/mqttService");

const feederRoutes = require("./routes/feederRoutes");
const scheduleRoutes = require("./routes/scheduleRoutes");

const app = express();
const httpServer = http.createServer(app);

const mqttWSPort = 8888;

app.use(cors());
app.use(express.json());
app.use(morgan("dev"));

app.use("/api/", feederRoutes);
app.use("/api/", scheduleRoutes);

startBroker(httpServer);

// El servidor HTTP principal expondrá tanto la API como el broker MQTT por WebSockets
httpServer.listen(mqttWSPort, () => {
  console.log(
    `🚀 Broker MQTT (WebSockets) and API listening on http://localhost:${mqttWSPort}`
  );
});
