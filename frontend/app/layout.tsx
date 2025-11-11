export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="es">
      <head>
        {/* Aquí puedes incluir metadatos, fuentes, etc. */}
      </head>
      <body>
        {/* Incluir la librería MQTT compatible con navegadores al inicio del body. */}
        {/* Esto asegura que la variable 'mqtt' esté disponible globalmente. */}
        <script src="https://unpkg.com/mqtt@4.3.7/dist/mqtt.min.js"></script>
        
        {/* Este es el contenido principal de la aplicación */}
        {children}
      </body>
    </html>
  );
}