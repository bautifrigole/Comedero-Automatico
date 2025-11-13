// frontend/app/page.js

"use client";

// La URL original es para HTTP. La usaremos para las peticiones de la API.
const backendHttpUrl = "http://192.168.1.42:8888";
// La URL para la conexión MQTT por WebSockets.
const backendWsUrl = "ws://192.168.1.42:8888";

import { useState, useEffect } from "react";
// Importar el nuevo componente
import ScheduleManager from "../components/ScheduleManager"; // Asumiendo que esta ruta es correcta

// --- Constantes MQTT ---
const DEVICE_ID = "Feeder-01";
const LEVEL_TOPIC = `${DEVICE_ID}/level`;

// Definir el tipo para el cliente MQTT global (asumiendo que se carga vía CDN)
type MqttClient = {
  connect: (url: string, options: any) => any;
  subscribe: (topic: string, callback: (err: any) => void) => void;
  on: (event: string, callback: (...args: any[]) => void) => void;
  end: () => void;
};
// -----------------------

export default function HomePage() {
  const [status, setStatus] = useState("Inactive");
  const [foodLevel, setFoodLevel] = useState("Unknown"); // Nuevo estado para el nivel del sensor
  const [isLoading, setIsLoading] = useState(false);
  const [isMounted, setIsMounted] = useState(false);

  // Función para inicializar la conexión MQTT
  const initializeMqttConnection = (mqttGlobal: MqttClient) => {
    let client: ReturnType<MqttClient["connect"]> | null = null;

    try {
      client = mqttGlobal.connect(backendWsUrl, {
        // Conexión WebSocket
        protocol: "ws",
        clientId: `feeder_frontend_${Math.random().toString(16).substr(2, 8)}`,
      });

      client.on("connect", () => {
        console.log("MQTT Client connected to broker via WebSocket.");

        client?.subscribe(LEVEL_TOPIC, (err: any) => {
          if (!err) {
            console.log(`Subscribed to ${LEVEL_TOPIC}`);
          }
        });
        client?.subscribe(`${DEVICE_ID}/status`, (err: any) => {
          if (!err) {
            console.log(`Subscribed to ${DEVICE_ID}/status`);
          }
        });
      });

      client.on("message", (topic: string, message: Buffer) => {
        const payload = message.toString();

        if (topic === LEVEL_TOPIC) {
          if (payload === "FOOD_LEVEL_LOW") {
            setFoodLevel("🔴 BAJO");
          } else if (payload === "FOOD_LEVEL_OK") {
            setFoodLevel("🟢 OK");
          }
        }

        if (topic === `${DEVICE_ID}/status`) {
          if (payload === "RATION_COMPLETE") {
            setStatus("✅ Ración de comida completada.");
            setTimeout(() => setStatus("Inactive"), 5000);
          } else if (payload === "DISPENSE_STARTED") {
            setStatus("🍽️ Dispensando comida...");
          }
        }
      });

      client.on("error", (err: Error) => {
        console.error("MQTT Connection Error:", err);
        setFoodLevel("❌ Error de Conexión MQTT");
      });
    } catch (e) {
      console.error("Failed to set up MQTT client:", e);
      setFoodLevel("❌ Error de inicialización de MQTT");
    }

    return client;
  };

  useEffect(() => {
    setIsMounted(true);
    let mqttClient: ReturnType<MqttClient["connect"]> | null = null;
    let checkInterval: NodeJS.Timeout;

    // --- Mecanismo de Polling (Espera) ---
    checkInterval = setInterval(() => {
      const mqttGlobal: MqttClient | undefined = (window as any).mqtt;

      if (mqttGlobal) {
        clearInterval(checkInterval); // Detener la verificación
        mqttClient = initializeMqttConnection(mqttGlobal);
      } else {
        // console.log("Waiting for MQTT library..."); // Descomentar para debug
        setFoodLevel("... Conectando MQTT ...");
      }
    }, 100); // Verificar cada 100ms

    return () => {
      // Limpiar el intervalo y la conexión al desmontar
      clearInterval(checkInterval);
      if (mqttClient) {
        mqttClient.end();
        console.log("MQTT Client disconnected.");
      }
    };
  }, []); // El array vacío asegura que esto corra solo una vez.

  // --- Lógica de Botones (Sin cambios) ---
  const handleDispenseClick = async () => {
    setIsLoading(true);
    setStatus("Enviando orden de dispensar...");

    try {
      const response = await fetch(`${backendHttpUrl}/api/dispense`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ device: DEVICE_ID }),
      });

      if (response.ok) {
        setStatus("✅ Orden enviada. Esperando confirmación...");
      } else {
        setStatus("❌ Error enviando orden.");
      }
    } catch (error) {
      console.error("Connection error:", error);
      setStatus("❌ No se pudo conectar al servidor.");
    } finally {
      setIsLoading(false);
    }
  };

  const handleStartMotorClick = async () => {
    setIsLoading(true);
    setStatus("Starting motor...");

    try {
      const response = await fetch(`${backendHttpUrl}/api/start-motor`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ device: "Feeder-01" }),
      });

      if (response.ok) {
        setStatus("✅ Motor started!");
      } else {
        setStatus("❌ Error starting motor.");
      }
    } catch (error) {
      console.error("Connection error:", error);
      setStatus("❌ Could not connect to server.");
    } finally {
      setIsLoading(false);
      setTimeout(() => setStatus("Motor running..."), 3000);
    }
  };

  const handleStopMotorClick = async () => {
    setIsLoading(true);
    setStatus("Stopping motor...");

    try {
      const response = await fetch(`${backendHttpUrl}/api/stop-motor`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ device: "Feeder-01" }),
      });

      if (response.ok) {
        setStatus("✅ Motor stopped!");
      } else {
        setStatus("❌ Error stopping motor.");
      }
    } catch (error) {
      console.error("Connection error:", error);
      setStatus("❌ Could not connect to server.");
    } finally {
      setIsLoading(false);
      setTimeout(() => setStatus("Inactive"), 3000);
    }
  };

  if (!isMounted) {
    return null;
  }

  // Estilo para el estado del nivel de comida
  const foodLevelStyle = (level: string) => {
    const base = "font-bold p-2 rounded-lg text-white shadow-md";
    if (level.includes("BAJO")) return `${base} bg-red-600`;
    if (level.includes("OK")) return `${base} bg-green-600`;
    return `${base} bg-gray-500`;
  };

  return (
    <main
      style={{
        fontFamily: "Arial",
        textAlign: "center",
        marginTop: "30px",
        padding: "20px",
        backgroundColor: "#f7f7f7",
        minHeight: "100vh",
      }}
    >
      <h1>🐾 Feeder Control</h1>

      {/* Indicador del Nivel de Comida */}
      <div
        style={{
          display: "flex",
          justifyContent: "center",
          gap: "20px",
          marginBottom: "30px",
          alignItems: "center",
        }}
      >
        <p style={{ fontSize: "18px" }}>
          Status: <strong>{status}</strong>
        </p>
        <div
          style={{ width: "2px", height: "30px", backgroundColor: "#ccc" }}
        ></div>
        <p style={{ fontSize: "18px" }}>
          Nivel Comida:
          <span
            className={foodLevelStyle(foodLevel)}
            style={{ marginLeft: "10px" }}
          >
            {foodLevel}
          </span>
        </p>
      </div>

      {/* Componente para gestionar los horarios de comida y agua */}
      <ScheduleManager backendUrl={backendHttpUrl} />

      {/* Botones de control manual */}
      <div
        style={{
          display: "flex",
          flexDirection: "column",
          gap: "20px",
          maxWidth: "400px",
          margin: "30px auto 0 auto",
        }}
      >
        <button
          onClick={handleDispenseClick}
          disabled={isLoading}
          style={{
            padding: "20px 40px",
            fontSize: "24px",
            cursor: isLoading ? "not-allowed" : "pointer",
            backgroundColor: isLoading ? "#ccc" : "#0070f3",
            color: "white",
            border: "none",
            borderRadius: "10px",
            transition: "background-color 0.3s",
            opacity: isLoading ? 0.5 : 1,
          }}
        >
          {isLoading ? "Processing..." : "🍽️ Dispense Food"}
        </button>

        <button
          onClick={handleStartMotorClick}
          disabled={isLoading}
          style={{
            padding: "20px 40px",
            fontSize: "20px",
            cursor: isLoading ? "not-allowed" : "pointer",
            backgroundColor: isLoading ? "#ccc" : "#10b981",
            color: "white",
            border: "none",
            borderRadius: "10px",
            transition: "background-color 0.3s",
            opacity: isLoading ? 0.5 : 1,
          }}
        >
          {isLoading ? "Processing..." : "▶️ Start Motor"}
        </button>

        <button
          onClick={handleStopMotorClick}
          disabled={isLoading}
          style={{
            padding: "20px 40px",
            fontSize: "20px",
            cursor: isLoading ? "not-allowed" : "pointer",
            backgroundColor: isLoading ? "#ccc" : "#ef4444",
            color: "white",
            border: "none",
            borderRadius: "10px",
            transition: "background-color 0.3s",
            opacity: isLoading ? 0.5 : 1,
          }}
        >
          {isLoading ? "Processing..." : "⏹️ Stop Motor"}
        </button>
      </div>
    </main>
  );
}
