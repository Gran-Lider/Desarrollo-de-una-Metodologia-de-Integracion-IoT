import sys
from PyQt6.QtCore import QUrl
from PyQt6.QtWidgets import QApplication, QMainWindow
from PyQt6.QtWebEngineWidgets import QWebEngineView
from PyQt6.QtWebEngineCore import QWebEngineProfile, QWebEnginePage

class VisorTesis(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("Sistema de Supervisión - Máquina dosificadora")

        # --- NAVEGADOR ---
        self.browser = QWebEngineView()
        
        # Perfil con memoria (cookies)
        self.perfil = QWebEngineProfile("CacheTesis", self.browser)
        self.perfil.setPersistentCookiesPolicy(QWebEngineProfile.PersistentCookiesPolicy.ForcePersistentCookies)
        pagina_web = QWebEnginePage(self.perfil, self.browser)
        self.browser.setPage(pagina_web)

        # --- CONEXIÓN DEL TRUCO ---
        # Cuando la página diga "Ya cargué", ejecutamos nuestra función
        self.browser.loadFinished.connect(self.inyectar_trucos_visuales)

        # --- TU ENLACE ---
        # Pon aquí tu enlace (asegúrate de que sea el correcto)
        mi_url = "https://thingsboard.cloud/dashboards/all/e1a77d10-e83b-11f0-a6fc-1dffa956f056"
        
        self.browser.setUrl(QUrl(mi_url))
        self.setCentralWidget(self.browser)

    def inyectar_trucos_visuales(self, ok):
        if ok:
            # Este script espera 3 segundos (3000 ms) a que cargue todo lo visual
            # y luego borra las barras a la fuerza.
            script_limpieza = """
            setTimeout(function() {
                // 1. Crear una hoja de estilo para ocultar las barras
                var estilo = document.createElement('style');
                estilo.innerHTML = `
                    /* Oculta la barra verde superior (Header) */
                    header, .tb-header, mat-toolbar.mat-primary { 
                        display: none !important; 
                    }
                    
                    /* Oculta la barra del título del dashboard (donde está el botón de editar) */
                    .tb-dashboard-toolbar, tb-dashboard-toolbar { 
                        display: none !important; 
                    }
                    
                    /* Oculta la barra lateral si aparece */
                    mat-sidenav { 
                        display: none !important; 
                    }
                    
                    /* Elimina márgenes extra para que ocupe todo el espacio */
                    .mat-drawer-content { 
                        margin-left: 0px !important; 
                        height: 100vh !important;
                    }
                    
                    /* Fuerza al contenido del dashboard a subir */
                    .tb-dashboard-content {
                        height: 100vh !important;
                        padding-top: 0px !important;
                    }
                `;
                document.head.appendChild(estilo);
                
                // Opcional: Intenta hacer clic en el botón de pantalla completa si existe
                // (Por si el estilo no fuera suficiente)
                var botones = document.querySelectorAll('button mat-icon');
                botones.forEach(function(icono) {
                    if (icono.innerText === 'fullscreen') {
                        icono.closest('button').click();
                    }
                });
                
            }, 2000); // <-- Espera 3 segundos. Si tu PC es lenta, sube esto a 5000
            """
            self.browser.page().runJavaScript(script_limpieza)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = VisorTesis()
    window.showMaximized()
    sys.exit(app.exec())