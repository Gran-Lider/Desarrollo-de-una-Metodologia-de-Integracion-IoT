# Desarrollo de una Metodología de Integración IoT para la Digitalización de una Máquina Dosificadora

Este repositorio contiene el desarrollo técnico y metodológico de un proyecto de integración IoT aplicado a una máquina dosificadora utilizada en un entorno de laboratorio agroalimentario (AGRO5). El trabajo se enfoca en la **digitalización, supervisión y control remoto** del equipo sin alterar su lógica original de funcionamiento.

El proyecto forma parte de un **trabajo de titulación** y propone una metodología replicable para la adaptación de equipos industriales de pequeña y mediana escala mediante tecnologías IoT.

---

## 🎯 Objetivo del proyecto

Desarrollar una **metodología de integración IoT mínimamente intrusiva** que permita:

- Capturar datos operativos de una máquina dosificadora autónoma.
- Transmitir dichos datos de forma inalámbrica a una plataforma IoT.
- Visualizar y almacenar información en tiempo real e histórica.
- Permitir control bidireccional remoto manteniendo la operación manual original.
- Establecer un modelo replicable para otros equipos industriales del laboratorio.

---

## 🏗️ Arquitectura general del sistema

El sistema está compuesto por los siguientes elementos:

- **Máquina dosificadora original** (controlada por microcontrolador STC con lógica TTL 5V).
- **HUB IoT** basado en un **ESP32**, actuando como intermediario (Man-in-the-Middle).
- **Plataforma IoT ThingsBoard (Cloud)** para visualización, almacenamiento y control.
- **Dashboard web** accesible desde el HMI del laboratorio.

La integración se realiza sin modificar la electrónica interna del equipo, utilizando técnicas de lectura sincronizada y pulsos de control.

---

## ⚙️ Componentes principales

### 🔹 Hardware
- ESP32 (SoC WiFi/Bluetooth)
- Módulos de expansión I/O (PCF8574)
- Etapas de acondicionamiento de señal (TTL 5V → 3.3V)
- Conexión no intrusiva a líneas de displays y pulsadores

### 🔹 Software embebido
- Lectura sincronizada de displays multiplexados (TDM ~120 Hz)
- Decodificación de segmentos (Presetting, Peso, Contador)
- Detección de pulsadores mediante línea común
- Lógica de control por máquina de estados
- Algoritmo de carga automática (modo Turbo / Precisión)
- Comunicación MQTT con ThingsBoard
- Manejo de RPC para control remoto

### 🔹 Plataforma IoT
- Registro del dispositivo en ThingsBoard
- Publicación de telemetría en formato JSON
- Dashboards interactivos para:
  - Visualización de valores actuales
  - Históricos de peso y cantidad
  - Control remoto (Iniciar, Parar, Velocidad, Preset)
  - Estado de conexión del equipo

---

## 📊 Variables monitoreadas

- **Presetting** (valor configurado)
- **Weight (g)** – Peso dosificado
- **Quantity** – Número de bolsas / ciclos
- Estado operativo (Parado, Operando, Vaciando)
- Eventos de conexión (Online / Offline)

---

## 🔁 Control bidireccional

El sistema permite:
- Control local mediante pulsadores físicos.
- Control remoto desde ThingsBoard mediante RPC.
- Prioridad a la interacción humana local.
- Ejecución de acciones físicas mediante pulsos controlados, emulando pulsaciones reales.

---

## ✅ Resultados principales

- Integración **reversible y no intrusiva**.
- Concordancia total entre datos físicos y digitales.
- Operación estable en pruebas prolongadas.
- Latencias del orden de milisegundos.
- Viabilidad técnica y económica frente a soluciones industriales tradicionales.

---

## 📌 Alcance y replicabilidad

La metodología desarrollada puede aplicarse a:
- Equipos industriales de pequeña escala.
- Laboratorios académicos.
- PYMES agroindustriales.
- Procesos que requieran trazabilidad y supervisión remota.

---

## 👨‍💻 Autores

- **Alex Guerrero**
- **José Nivicela**

Ingeniería en Telecomunicaciones  
Universidad de Cuenca

---

## 📄 Licencia

Este proyecto tiene fines **académicos y de investigación**.  
El uso del contenido debe ser referenciado adecuadamente.
