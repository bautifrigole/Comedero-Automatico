// frontend/app/page.js

'use client'; 

import { useState, useEffect } from 'react';

export default function HomePage() {
  const [status, setStatus] = useState('Inactivo');
  const [isLoading, setIsLoading] = useState(false);
  const [isMounted, setIsMounted] = useState(false);

  // useEffect se ejecuta solo en el cliente, después del primer renderizado.
  useEffect(() => {
    setIsMounted(true);
  }, []);

  const handleDispensarClick = async () => {
    setIsLoading(true);
    setStatus('Enviando orden...');

    try {
      const response = await fetch('http://localhost:3001/api/dispensar', {
        method: 'POST',
      });

      if (response.ok) {
        setStatus('✅ ¡Orden enviada con éxito!');
      } else {
        setStatus('❌ Error al enviar la orden.');
      }
    } catch (error) {
      console.error('Error de conexión:', error);
      setStatus('❌ No se pudo conectar al servidor.');
    } finally {
      setIsLoading(false);
      setTimeout(() => setStatus('Inactivo'), 3000);
    }
  };

  // No renderizar nada en el servidor o en el primer render del cliente
  // para evitar la discrepancia.
  if (!isMounted) {
    return null; // O puedes retornar un esqueleto de carga (loading skeleton)
  }

  return (
    <main style={{ fontFamily: 'Arial', textAlign: 'center', marginTop: '100px' }}>
      <h1>🐾 Control del Comedero</h1>
      <p>Estado: {status}</p>
      <button
        onClick={handleDispensarClick}
        disabled={isLoading}
        style={{
          padding: '20px 40px',
          fontSize: '24px',
          cursor: 'pointer',
          backgroundColor: isLoading ? '#ccc' : '#0070f3',
          color: 'white',
          border: 'none',
          borderRadius: '10px',
        }}
      >
        {isLoading ? 'Dispensando...' : 'Dispensar Comida'}
      </button>
    </main>
  );
}