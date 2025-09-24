# 🐾 Comedero Automático para Perros y Gatos

Este proyecto tiene como objetivo diseñar y construir un **comedero automático inteligente** para mascotas (perros o gatos), utilizando una **placa ESP32** como controlador principal. 

El sistema busca brindar una solución práctica para la **alimentación e hidratación programada y remota** de las mascotas, garantizando comodidad al dueño y regularidad en la dieta del animal.

---

## 🎯 Objetivos del proyecto

- **Dispensar alimento automáticamente** en base a horarios configurados. 
- **Proveer agua de forma automática** mediante un dispenser con bomba. 
- **Permitir control remoto** mediante conexión WiFi (Arduino IoT Cloud u otra plataforma). 
- **Monitorear el estado del comedero y bebedero**, detectando si el plato de comida o agua está lleno o vacío. 
- Ofrecer una **interfaz sencilla para el usuario**, ya sea desde una aplicación, dashboard web o notificaciones móviles. 

---

## ⚙️ Componentes principales

- **ESP32**: microcontrolador con conectividad WiFi. 
- **Contenedor de alimento**: depósito principal donde se almacena la comida seca. 
- **Mecanismo dispensador de alimento**: accionado por un motor o servomotor. 
- **Dispenser de agua**: incluye una **bomba pequeña** para llenar el plato de forma controlada. 
- **Plato receptor**: recibe tanto el alimento como el agua. 
- **Sensores**: 
  - Sensor de peso para verificar si hay comida en el plato. 
  - Sensor de peso adicional o celda de carga para verificar si el plato contiene agua suficiente. 
  - Microswitch para detectar el giro del motor del dispensador. 
- **Módulo de relé**: para controlar la bomba de agua y el motor del dispenser. 
- **Fuente de alimentación**: adecuada para la ESP32, el motor y la bomba de agua. 

---

## 📐 Funcionamiento general

1. El usuario configura los **horarios de alimentación y dispensado de agua** desde la plataforma conectada a internet. 
2. La **ESP32** procesa esta información y controla tanto el motor del dispenser de comida como la bomba de agua. 
3. Si es la hora indicada (o se solicita manualmente desde la app), el sistema libera una ración de alimento y/o agua en el plato. 
4. Los **sensores de peso** verifican si hay alimento o agua en el plato, evitando que se dispensen raciones innecesarias. 
5. El usuario puede **monitorear en tiempo real** el estado del comedero y bebedero, así como el historial de dispensado. 

