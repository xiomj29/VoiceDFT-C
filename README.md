# Analizador de Voz en C mediante DFT

Este proyecto implementa un **procesador de voz en lenguaje C**, capaz de grabar audio desde el micrófono o generar una señal de ejemplo, aplica la **Transformada Discreta de Fourier (DFT)** y detecta la **frecuencia fundamental** de la voz humana para clasificarla como grave, media o aguda.

> Compatible con sistemas Linux. Requiere `arecord` de ALSA para grabación en tiempo real.

---

## Características

- Grabación de 3 segundos de audio a 8 kHz.
- Normalización y filtrado de la señal.
- Generación de señal de voz sintética si no hay micrófono.
- Cálculo de la Transformada Discreta de Fourier (DFT).
- Identificación de la frecuencia principal de la señal (pitch).
- Clasificación del tipo de voz:
  - Voz muy grave
  - Voz grave
  - Voz media
  - Voz aguda
  - Voz muy aguda
- Exportación de resultados en archivos `.csv`:
  - `muestras_audio.csv`
  - `analisis_dft.csv`

---

## Fundamento teórico

El análisis de la voz se basa en:

- **Ventana de Hann** para reducir fugas espectrales.
- **DFT** implementada manualmente para convertir la señal del dominio del tiempo al dominio de la frecuencia.
- **Análisis de magnitudes espectrales** para determinar la frecuencia con mayor energía entre 80 Hz y 300 Hz (rango típico de la voz humana).
- Clasificación del tipo de voz según la frecuencia fundamental detectada.

---

## Requisitos

- Linux (se probó en Fedora, pero puede adaptarse a otras distros)
- Paquete ALSA (para el comando `arecord`)
- Compilador GCC

### Instalación de dependencias (Fedora)

```bash
sudo apt install alsa-utils
```
---
### Compilación y Ejecución (Fedora)

```bash
gcc DFT.c -o analizador -lm
./analizador
```
---
