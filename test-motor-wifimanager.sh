#!/bin/bash

# Script para probar los comandos del comedero con WiFiManager

BACKEND_URL="http://localhost:3001"
DEVICE_NAME=${1:-"Comedero-01"}  # Usar primer parámetro o default

echo "🎯 Probando comandos para dispositivo: $DEVICE_NAME"
echo "📡 Backend: $BACKEND_URL"
echo ""

# Función para enviar comando
send_command() {
    local endpoint=$1
    local command_name=$2
    
    echo "🔄 Enviando: $command_name"
    
    response=$(curl -s -X POST "$BACKEND_URL/api/$endpoint" \
        -H "Content-Type: application/json" \
        -d "{\"device\": \"$DEVICE_NAME\"}")
    
    if [ $? -eq 0 ]; then
        echo "✅ $command_name: $response"
    else
        echo "❌ Error enviando $command_name"
    fi
    echo ""
}

# Función para listar dispositivos
list_devices() {
    echo "📋 Dispositivos conectados:"
    curl -s "$BACKEND_URL/api/dispositivos" | jq '.' 2>/dev/null || echo "Instala jq para ver formato JSON bonito"
    echo ""
}

echo "1. Listando dispositivos conectados..."
list_devices

echo "2. Iniciando motor..."
send_command "iniciar-motor" "INICIAR_MOTOR"

echo "3. Esperando 3 segundos..."
sleep 3

echo "4. Deteniendo motor..."
send_command "detener-motor" "DETENER_MOTOR"

echo "5. Dispensando comida (modo automático)..."
send_command "dispensar" "DISPENSAR_COMIDA"

echo "✅ Prueba completada"
echo ""
echo "💡 Para usar con otro dispositivo:"
echo "   ./test-motor-wifimanager.sh Comedero-Cocina"