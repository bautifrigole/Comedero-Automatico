# 📱 Comedero Automático con WiFiManager

## 🎯 Configuración Fácil de WiFi

Ya no necesitas reprogramar el ESP32 cada vez que quieras cambiar las credenciales WiFi. Ahora el sistema usa un **portal cautivo** para configurar todo desde tu celular o computadora.

## 🚀 Cómo usar el nuevo sistema

### 1. Primera vez / Sin configuración WiFi

1. **Carga el código** `PruebaBroker_WiFiManager.ino` en tu ESP32
2. **Reinicia el ESP32** (o desconéctalo y conéctalo)
3. **Busca la red WiFi** `Comedero-Setup` desde tu celular/computadora
4. **Conéctate** usando la contraseña: `12345678`
5. **Se abrirá automáticamente** una página web (si no, ve a `192.168.4.1`)
6. **Configura**:
   - **WiFi SSID**: Tu red WiFi
   - **WiFi Password**: Contraseña de tu red
   - **Servidor MQTT**: IP del backend (ej: `192.168.247.246`)
   - **Puerto MQTT**: `1883`
   - **Nombre del dispositivo**: `Comedero-01` (o el que quieras)
7. **Haz clic en "Save"**
8. **El ESP32 se reiniciará** y se conectará automáticamente

### 2. Cambiar configuración posteriormente

**Método 1 - Botón BOOT:**

1. **Mantén presionado el botón BOOT** del ESP32 por 3 segundos
2. **Se reseteará** la configuración WiFi
3. **Sigue los pasos del punto 1**

**Método 2 - Borrar desde código:**

1. Descomenta la línea `wm.resetSettings();` en el código
2. Carga el código
3. La configuración se borrará automáticamente

## 📋 Librerías necesarias

Instala estas librerías en el Arduino IDE:

```
1. WiFiManager by tzapu (versión 2.0.16 o superior)
2. PubSubClient by Nick O'Leary
3. ArduinoJson (dependencia de WiFiManager)
```

### Cómo instalar librerías:

1. **Arduino IDE** → **Herramientas** → **Administrar librerías**
2. **Busca** cada librería por nombre
3. **Instala** la versión más reciente

## 🔧 Características del nuevo sistema

### ✅ Ventajas

- **Sin reprogramar**: Cambia WiFi desde tu celular
- **Portal web intuitivo**: Interfaz fácil de usar
- **Configuración persistente**: Se guarda en memoria del ESP32
- **Reseteo fácil**: Botón BOOT para reconfigurar
- **Soporte múltiples dispositivos**: Cada comedero puede tener nombre único
- **IP del servidor configurable**: Ya no hardcodeada

### 🎛️ Configuraciones disponibles

- **Red WiFi** (SSID y contraseña)
- **Servidor MQTT** (IP del backend)
- **Puerto MQTT** (por defecto 1883)
- **Nombre del dispositivo** (para identificar múltiples comederos)

## 🌐 Endpoints del backend actualizados

El backend ahora soporta múltiples dispositivos:

```bash
# Dispositivo por defecto (comedero/01)
curl -X POST http://localhost:3001/api/iniciar-motor

# Dispositivo específico
curl -X POST http://localhost:3001/api/iniciar-motor \
  -H "Content-Type: application/json" \
  -d '{"device": "Comedero-Cocina"}'

# Listar dispositivos conectados
curl http://localhost:3001/api/dispositivos
```

## 🔍 Solución de problemas

### El portal no aparece

- Verifica que no haya otra red "Comedero-Setup"
- Reinicia el ESP32 y espera 30 segundos
- Ve manualmente a `192.168.4.1` en tu navegador

### No se conecta al WiFi después de configurar

- Verifica que la contraseña WiFi sea correcta
- Asegúrate de que el ESP32 esté en rango de la red
- Revisa el monitor serie para ver errores

### MQTT no se conecta

- Verifica que la IP del servidor sea correcta
- Confirma que el backend esté ejecutándose
- Revisa que el puerto sea 1883

## 📱 Ejemplo de configuración

```
Red WiFi: MiCasa_WiFi
Contraseña WiFi: micontraseña123
Servidor MQTT: 192.168.1.100
Puerto MQTT: 1883
Nombre dispositivo: Comedero-Sala
```

Resultado: El dispositivo se conectará a `MiCasa_WiFi` y enviará mensajes a topics como `Comedero-Sala/estado` y recibirá órdenes en `Comedero-Sala/ordenes`.

## 🔄 Migración desde código anterior

Si ya tienes el código anterior funcionando:

1. **Respalda** tu configuración actual (SSID, password, IP)
2. **Carga** el nuevo código `PruebaBroker_WiFiManager.ino`
3. **Configura** usando el portal web con los mismos datos
4. **¡Listo!** Ya tienes configuración flexible

---

💡 **Tip**: Guarda la IP de tu backend y credenciales WiFi en un lugar seguro para futuras configuraciones.
