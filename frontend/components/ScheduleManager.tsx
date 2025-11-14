// components/ScheduleManager.tsx
"use client";

import { useState, useEffect } from "react";
import { Clock, Trash2, PlusCircle, Save } from "lucide-react";

interface ScheduleManagerProps {
  backendUrl: string;
}

export interface Schedule {
  feeding: string[];
  water: string[];
}

// Helper para fusionar clases de Tailwind (asumiendo que tienes una función cn o similar)
const cn = (...classes: (string | undefined)[]) =>
  classes.filter(Boolean).join(" ");

export default function ScheduleManager({ backendUrl }: ScheduleManagerProps) {
  const [schedule, setSchedule] = useState<Schedule>({
    feeding: [],
    water: [],
  });
  const [newFeedingTime, setNewFeedingTime] = useState("");
  const [newWaterTime, setNewWaterTime] = useState("");
  const [loading, setLoading] = useState(false);
  const [status, setStatus] = useState("");
  const [duration, setDuration] = useState(""); // Duración en segundos

  const fetchSchedule = async () => {
    try {
      setLoading(true);
      const response = await fetch(`${backendUrl}/api/schedule`);
      if (response.ok) {
        const data = await response.json();
        setSchedule(data.schedule);

        if (data.duration) {
          setDuration(String(data.duration)); 
        }
      } else {
        setStatus("❌ Error al obtener el horario.");
      }
    } catch (error) {
      setStatus("❌ No se pudo conectar a la API de horarios.");
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchSchedule();
  }, []);

  const handleAddSchedule = async (type: "feeding" | "water", time: string) => {
    if (!time || !/^\d{2}:\d{2}$/.test(time)) {
      setStatus("❌ Formato de hora inválido. Usa HH:MM.");
      return;
    }

    const currentSchedule = schedule[type];
    if (currentSchedule.includes(time)) {
      setStatus(
        `❌ El horario de ${
          type === "feeding" ? "comida" : "agua"
        } ya contiene ${time}.`
      );
      return;
    }

    // Ordenar el nuevo horario
    const newScheduleArray = [...currentSchedule, time].sort();

    const schedulesToUpdate = {
      feeding: type === "feeding" ? newScheduleArray : schedule.feeding,
      water: type === "water" ? newScheduleArray : schedule.water,
    };

    await updateSchedule(schedulesToUpdate, `Agregando ${time}...`);
  };

  const handleRemoveTime = async (
    type: "feeding" | "water",
    timeToRemove: string
  ) => {
    const newScheduleArray = schedule[type].filter((t) => t !== timeToRemove);

    const schedulesToUpdate = {
      feeding: type === "feeding" ? newScheduleArray : schedule.feeding,
      water: type === "water" ? newScheduleArray : schedule.water,
    };

    await updateSchedule(schedulesToUpdate, `Eliminando ${timeToRemove}...`);
  };

  const updateSchedule = async (
    schedulesToUpdate: Schedule,
    loadingMessage: string
  ) => {
    try {
      setLoading(true);
      setStatus(`🔄 ${loadingMessage}`);
      const response = await fetch(`${backendUrl}/api/set-schedule`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(schedulesToUpdate),
      });

      if (response.ok) {
        const data = await response.json();
        setSchedule(data.schedule);
        setStatus("✅ Horario actualizado exitosamente!");
        setNewFeedingTime("");
        setNewWaterTime("");
      } else {
        setStatus("❌ Error al actualizar el horario.");
      }
    } catch (error) {
      setStatus("❌ No se pudo conectar a la API de horarios.");
    } finally {
      setLoading(false);
      setTimeout(() => setStatus(""), 3000);
    }
  };

  const handleSetDuration = async () => {
    const durationNum = Number(duration);
    if (isNaN(durationNum) || durationNum <= 0 || durationNum > 60) {
      setStatus("❌ Duración inválida. Debe ser un número entre 1 y 60 segundos.");
      return;
    }

    try {
      setLoading(true);
      setStatus(`🔄 Estableciendo duración a ${durationNum}s...`);
      const response = await fetch(`${backendUrl}/api/set-duration`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ duration: durationNum }),
      });

      if (response.ok) {
        setStatus("✅ Duración de ración actualizada!");
      } else {
        setStatus("❌ Error al actualizar la duración.");
      }
    } catch (error) {
      setStatus("❌ No se pudo conectar a la API.");
    } finally {
      setLoading(false);
      setTimeout(() => setStatus(""), 3000);
    }
  };

  // Componente interno para renderizar los bloques de horario
  const ScheduleBlock = ({
    type,
    times,
  }: {
    type: "feeding" | "water";
    times: string[];
  }) => {
    const isFeeding = type === "feeding";
    const title = isFeeding ? "🍽️ Horario de Comida" : "💧 Horario de Agua";
    const accentBg = isFeeding ? "bg-blue-50/50" : "bg-green-50/50";
    const accentText = isFeeding ? "text-blue-700" : "text-green-700";
    const buttonBg = isFeeding
      ? "bg-blue-600 hover:bg-blue-700"
      : "bg-green-600 hover:bg-green-700";
    const newTime = isFeeding ? newFeedingTime : newWaterTime;
    const setNewTime = isFeeding ? setNewFeedingTime : setNewWaterTime;

    return (
      <div
        className={cn(
          "rounded-lg border border-border p-4 shadow-sm",
          accentBg
        )}
      >
        <h3 className="text-lg font-semibold text-foreground mb-3">{title}</h3>

        {/* Lista de Horarios */}
        <div className="flex flex-wrap gap-2 mb-4 min-h-[40px] items-center">
          {times.length > 0 ? (
            times.map((time) => (
              <div
                key={time}
                className={cn(
                  "flex items-center gap-1 rounded-full px-3 py-1 text-sm font-medium",
                  accentBg,
                  accentText,
                  "border border-current"
                )}
              >
                <Clock className="w-3 h-3" />
                {time}
                <button
                  onClick={() => handleRemoveTime(type, time)}
                  disabled={loading}
                  className="ml-1 text-red-600 hover:text-red-800 disabled:opacity-50 transition-colors"
                  aria-label={`Eliminar hora ${time}`}
                >
                  <Trash2 className="w-3 h-3" />
                </button>
              </div>
            ))
          ) : (
            <p className="text-muted-foreground text-sm italic">
              No hay horarios programados.
            </p>
          )}
        </div>

        {/* Añadir Nuevo Horario */}
        <div className="flex gap-2 mt-3">
          {/* Este es el Time Picker mejorado (input type="time" con estilos de Shadcn) */}
          <input
            type="time"
            value={newTime}
            onChange={(e) => setNewTime(e.target.value)}
            disabled={loading}
            // Clases de Tailwind basadas en la estética de tu aplicación (como en LoginForm)
            className="flex h-10 w-[120px] rounded-md border border-input bg-card px-3 py-2 text-sm
            placeholder:text-muted-foreground focus:ring-2 focus:ring-[var(--ring)] focus:border-blue-500 disabled:opacity-50"
          />
          <button
            onClick={() => handleAddSchedule(type, newTime)}
            disabled={loading || !newTime}
            className={cn(
              "flex items-center gap-1 h-10 px-4 py-2 text-sm font-medium rounded-md text-white transition-colors disabled:opacity-50",
              buttonBg
            )}
          >
            <PlusCircle className="w-4 h-4" />
            Agregar
          </button>
        </div>
      </div>
    );
  };

  return (
    <div className="max-w-xl mx-auto p-6 bg-card border border-border rounded-xl shadow-lg">
      <h2 className="text-2xl font-bold text-foreground mb-6 text-center">
        ⏰ Administrador de Horarios
      </h2>

      {status && (
        <div
          className={cn(
            "p-3 mb-4 rounded text-sm font-medium",
            status.startsWith("❌")
              ? "bg-red-100 text-red-700"
              : "bg-green-100 text-green-700"
          )}
        >
          {status}
        </div>
      )}

      <div className="space-y-4">
        <ScheduleBlock type="feeding" times={schedule.feeding} />
        {/* <ScheduleBlock type="water" times={schedule.water} /> */}
      </div>

      <div className="rounded-lg border border-border p-4 shadow-sm bg-card">
          <h3 className="text-lg font-semibold text-foreground mb-3">⏱️ Duración de Ración (segundos)</h3>
          <div className="flex gap-2">
            <input
              type="number"
              value={duration}
              onChange={(e) => setDuration(e.target.value)}
              disabled={loading}
              className="flex h-10 w-[120px] rounded-md border border-input bg-card px-3 py-2 text-sm
              placeholder:text-muted-foreground focus:ring-2 focus:ring-[var(--ring)] focus:border-blue-500 disabled:opacity-50"
              min="1"
              max="60"
            />
            <button
              onClick={handleSetDuration}
              disabled={loading || !duration}
              className={cn(
                "flex items-center gap-1 h-10 px-4 py-2 text-sm font-medium rounded-md text-white transition-colors disabled:opacity-50",
                "bg-indigo-600 hover:bg-indigo-700" // Color primario
              )}
            >
              <Save className="w-4 h-4" />
              Guardar
            </button>
          </div>
        </div>
    </div>
  );
}
