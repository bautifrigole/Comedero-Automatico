// frontend/app/page.js

"use client";

const backendUrl = "http://10.173.102.246:8888";

import { useState, useEffect } from "react";
// Importar el nuevo componente
import ScheduleManager from "../components/ScheduleManager";

export default function HomePage() {
  const [status, setStatus] = useState("Inactive");
  const [isLoading, setIsLoading] = useState(false);
  const [isMounted, setIsMounted] = useState(false);

  // useEffect runs only on the client, after the first render.
  useEffect(() => {
    setIsMounted(true);
  }, []);

  const handleDispenseClick = async () => {
    setIsLoading(true);
    setStatus("Sending dispense order...");

    try {
      const response = await fetch(`${backendUrl}/api/dispense`, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({ device: "Feeder-01" }),
      });

      if (response.ok) {
        setStatus("✅ Dispense order sent successfully!");
      } else {
        setStatus("❌ Error sending order.");
      }
    } catch (error) {
      console.error("Connection error:", error);
      setStatus("❌ Could not connect to server.");
    } finally {
      setIsLoading(false);
      setTimeout(() => setStatus("Inactive"), 3000);
    }
  };

  const handleStartMotorClick = async () => {
    setIsLoading(true);
    setStatus("Starting motor...");

    try {
      const response = await fetch(`${backendUrl}/api/start-motor`, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
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
      const response = await fetch(`${backendUrl}/api/stop-motor`, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
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

  // Don't render anything on the server or on the client's first render
  // to avoid discrepancies.
  if (!isMounted) {
    return null; // Or you can return a loading skeleton
  }

  return (
    <main
      style={{
        fontFamily: "Arial",
        textAlign: "center",
        marginTop: "30px", // Ajustado para el nuevo componente
        padding: "20px",
        backgroundColor: "#f7f7f7",
        minHeight: "100vh",
      }}
    >
      <h1>🐾 Feeder Control</h1>
      <p style={{ fontSize: "18px", marginBottom: "30px" }}>
        Status: <strong>{status}</strong>
      </p>

      {/* Componente para gestionar los horarios de comida y agua */}
      <ScheduleManager backendUrl={backendUrl} />

      {/* Botones de control manual */}
      <div
        style={{
          display: "flex",
          flexDirection: "column",
          gap: "20px",
          maxWidth: "400px",
          margin: "30px auto 0 auto", // Margen superior para separarlo del Schedule Manager
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
