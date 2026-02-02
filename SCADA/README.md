# Sistema de Supervisión HMI - Máquina Dosificadora

Este repositorio contiene el código fuente de la interfaz de supervisión de escritorio (HMI) desarrollada como parte del proyecto de tesis: **"Diseño y construcción de una máquina dosificadora controlada vía IoT"**.

La aplicación actúa como un contenedor nativo (wrapper) para visualizar el dashboard de **ThingsBoard**, proporcionando un entorno libre de distracciones (Modo Kiosco) y optimizado para la operación en planta.

## 📋 Características

* **Entorno Standalone:** Ejecuta el panel de control como una aplicación de escritorio independiente, sin barras de navegación de navegador web ni distracciones.
* **Modo Kiosco Forzado:** Implementa algoritmos de inyección de JavaScript para ocultar automáticamente las barras laterales y cabeceras nativas de ThingsBoard, maximizando el área de visualización.
* **Gestión de Sesiones Persistente:** Almacena cookies y tokens de autenticación localmente, permitiendo que el operador inicie sesión una única vez.
* **Renderizado Chromium:** Utiliza el motor `QtWebEngine` (basado en Chromium) para asegurar compatibilidad total con los gráficos modernos de ThingsBoard.

## 🛠️ Requisitos del Sistema

* **Sistema Operativo:** Windows 10/11 (x64).
* **Lenguaje:** Python 3.10 o superior.
* **Librerías:** PyQt6.

## 🚀 Instalación y Ejecución (Entorno de Desarrollo)

Si deseas ejecutar el código fuente directamente o realizar modificaciones:

1.  **Clonar el repositorio:**
    ```bash
    git clone [https://github.com/TU_USUARIO/nombre-repo.git](https://github.com/TU_USUARIO/nombre-repo.git)
    cd nombre-repo
    ```

2.  **Crear un entorno virtual (Opcional pero recomendado):**
    ```bash
    python -m venv venv
    .\venv\Scripts\activate
    ```

3.  **Instalar dependencias:**
    ```bash
    pip install PyQt6 PyQt6-WebEngine pyinstaller
    ```

4.  **Ejecutar la aplicación:**
    ```bash
    python main.py
    ```

## 📦 Generación del Ejecutable (.exe)

Para desplegar la aplicación en la computadora final (sin necesidad de instalar Python), se utiliza **PyInstaller**. Ejecuta el siguiente comando en la terminal:

```bash
pyinstaller --noconsole --onefile --clean --name="SupervisorDosificadora" main.py
