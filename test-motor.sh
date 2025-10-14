#!/bin/bash

echo "🟢 Iniciando motor..."
curl -X POST http://localhost:3001/api/iniciar-motor
echo ""

echo "⏳ Esperando 5 segundos..."
sleep 5

echo "🔴 Deteniendo motor..."
curl -X POST http://localhost:3001/api/detener-motor
echo ""

echo "✅ Prueba completada"