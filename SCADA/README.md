# SCADA / HMI – Aplicación ejecutable (.exe) complementaria

Como parte **complementaria** de este trabajo, se ha desarrollado una **aplicación ejecutable (.exe)** para la pantalla **HMI** con el objetivo de **tener un acceso más rápido a la plataforma** y **mejorar la interacción con el usuario**.

Para ello, se utilizó el lenguaje de programación **Python** y el framework **Qt** a través de la librería **PyQt6**. Esta solución integra un motor de navegación web **Chromium** mediante el módulo **QWebEngine**, lo que permite **incrustar contenido web** dentro de aplicaciones Qt para su visualización en una ventana dedicada en la HMI.

---

## 📁 Contenido de la carpeta

- `main.py`: código fuente principal del launcher HMI.
- `requirements.txt`: dependencias del proyecto.
- `icono2-app.ico`: ícono de la aplicación (opcional).
- `.gitignore`: exclusiones para evitar subir artefactos generados (ej. `dist/`, `build/`).
- `README.md`: documentación de ejecución y generación del ejecutable.

> **Nota:** no se recomienda versionar las carpetas `dist/` y `build/` porque se generan automáticamente al compilar con PyInstaller.

---

## ✅ Requisitos

- **Sistema Operativo:** Windows 10/11 (x64)
- **Python:** 3.10 o superior
- **Dependencias principales:** `PyQt6` y `PyQt6-WebEngine` (QWebEngine / Chromium)

---

## 🚀 Instalación y Ejecución (Entorno de Desarrollo)

Si deseas ejecutar el código fuente directamente o realizar modificaciones:

1.  **Clonar el repositorio:**
    ```bash
    git clone https://github.com/Gran-Lider/Desarrollo-de-una-Metodologia-de-Integracion-IoT.git
    cd Desarrollo-de-una-Metodologia-de-Integracion-IoT/SCADA
    ```

2.  **Crear un entorno virtual (Opcional pero recomendado):**
    ```bash
    python -m venv venv
    .\venv\Scripts\activate
    ```

3.  **Instalar dependencias:**
    ```bash
    pip install -r requirements.txt
    ```

4.  **Ejecutar la aplicación:**
    ```bash
    python main.py
    ```

---

## 📦 Generación del Ejecutable (.exe)

Para desplegar la aplicación en la computadora final (sin necesidad de instalar Python), se utiliza **PyInstaller**. Ejecuta el siguiente comando en la terminal **desde la carpeta `SCADA/`**:

```bash
pyinstaller --noconsole --onefile --clean --icon="icono2-app.ico" --name="Máquina Dosificadora IoT" main.py
```

## 🧳 Portable

La salida del código anterior es un archivo con extensión **(.exe)** que se puede llevar en una memoria USB y ejecutarlo desde cualquier ordenador que cumpla con los requisitos antes expuestos.

### Vista del ejecutable

![Uploading image.png…]()


