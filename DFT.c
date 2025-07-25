#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846
#define FRECUENCIA_MUESTREO 8000  // 8kHz, suficiente para voz humana
#define TIEMPO_GRABACION 3         // 3 segundos de grabación
#define UMBRAL_SILENCIO 0.0003     // Mínima energía para considerar voz
#define UMBRAL_VOZ 100             // Sensibilidad para detectar voz (mas de 300 no lo detecta)

// Estructura para guardar la señal de voz
typedef struct {
    double *muestras;  // Valores de audio (-1 a 1)
    int num_muestras;  // Cantidad total de muestras
} SenalVoz;
// Compara dos números para ordenarlos
int comparar_doubles(const void *a, const void *b) {
    double num1 = *(const double*)a;
    double num2 = *(const double*)b;
    return (num1 > num2) - (num1 < num2);
}

// Guarda las muestras de audio en un archivo CSV
void guardar_muestras(SenalVoz *senal, const char *nombre_archivo) {
    FILE *archivo = fopen(nombre_archivo, "w");
    if (!archivo) {
        printf("Error al crear %s\n", nombre_archivo);
        return;
    }
    
    // Encabezado del archivo
    fprintf(archivo, "Muestras de audio normalizadas (-1 a 1)\n");
    fprintf(archivo, "Muestras: %d, Frecuencia de muestreo: %d Hz\n", senal->num_muestras, FRECUENCIA_MUESTREO);
    fprintf(archivo, "indice,valor\n");
    
    // Datos: indice y valor de cada muestra 
    for (int i = 0; i < senal->num_muestras; i++) {
        fprintf(archivo, "%d,%.6f\n", i, senal->muestras[i]);
    }
    fclose(archivo);
    printf("Muestras guardadas en '%s'\n", nombre_archivo);
}

// Guarda el análisis de frecuencias para la dft en csv
void guardar_resultados_dft(double *magnitudes, int num_frecuencias, double frecuencia_principal, const char *nombre_archivo) {
    FILE *archivo = fopen(nombre_archivo, "w");
    if (!archivo) {
        printf("Error al crear %s\n", nombre_archivo);
        return;
    }
    
    // Inicio del programa
    fprintf(archivo, "Analisis de frecuencias (DFT)\n");
    fprintf(archivo, "Frecuencia principal detectada: %.2f Hz\n", frecuencia_principal);
    fprintf(archivo, "indice_frecuencia,frecuencia_hz,magnitud\n");
    
    // Datos: indice, frecuencia en hz, magnitud 
    for (int k = 1; k < num_frecuencias; k++) {
        double frecuencia = (double)k * FRECUENCIA_MUESTREO / (num_frecuencias * 2);
        fprintf(archivo, "%d,%.2f,%.2f\n", k, frecuencia, magnitudes[k]);
    }
    fclose(archivo);
    printf("Analisis DFT guardado en '%s'\n", nombre_archivo);
}

// Genera una señal de voz de ejemplo (si no se detecta micrófono)
SenalVoz* generar_voz_ejemplo() {
    printf("Usando señal de ejemplo (no se detectó micrófono)\n");
    
    SenalVoz *senal = malloc(sizeof(SenalVoz));
    senal->num_muestras = FRECUENCIA_MUESTREO * TIEMPO_GRABACION;
    senal->muestras = malloc(senal->num_muestras * sizeof(double));
    
    // Simula una voz con frecuencia variable entre 130 hz y 230 hz 
    for (int i = 0; i < senal->num_muestras; i++) {
        double tiempo = (double)i / FRECUENCIA_MUESTREO;
        double frecuencia = 180 + 50 * sin(2 * PI * 0.5 * tiempo);
        
        // Genera armónicos para sonido más natural
        senal->muestras[i] = 0.5 * sin(2 * PI * frecuencia * tiempo) +
                             0.3 * sin(2 * PI * 2 * frecuencia * tiempo) +
                             0.1 * sin(2 * PI * 3 * frecuencia * tiempo);
        
        // Aplica un fadeout para que suene realista
        double fade = exp(-2.0 * tiempo / TIEMPO_GRABACION);
        senal->muestras[i] *= fade;
    }
    
    return senal;
}

// Intenta grabar voz desde el micrófono
SenalVoz* grabar_voz() {
    printf("Grabando %d segundos... Por favor comienza a hablar\n", TIEMPO_GRABACION);
    
    // Comando para grabar en formato RAW (16-bit, 8kHz) para linux fedora 
    // !PENDIENTE CHECAR SI SIRVE EN OTRAS DISTRIBUCIONES Y LAPTOPS¡

    system("timeout 3 arecord -r 8000 -f S16_LE -t raw > grabacion.raw 2>/dev/null");
    
    // Abre el archivo grabado previamente
    FILE *archivo = fopen("grabacion.raw", "rb");
    if (!archivo) {
        printf("Error al grabar. No se ha detectado un microfono\n");
        return NULL;
    }
    
    // Calcula el tamaño del archivo
    fseek(archivo, 0, SEEK_END);
    long tamano = ftell(archivo);
    fseek(archivo, 0, SEEK_SET);
    
    // Lee los datos en formato short (16-bit)
    int num_muestras = tamano / sizeof(short);
    short *datos_raw = malloc(num_muestras * sizeof(short));
    fread(datos_raw, sizeof(short), num_muestras, archivo);
    fclose(archivo);
    
    // Convierte a double (-1.0 a 1.0) y aplica filtro simple
    SenalVoz *senal = malloc(sizeof(SenalVoz));
    senal->num_muestras = num_muestras;
    senal->muestras = malloc(num_muestras * sizeof(double));
    
    double max_valor = 0.0001; // Evita división por cero
    
    for (int i = 0; i < num_muestras; i++) {
        senal->muestras[i] = (double)datos_raw[i] / 32768.0; // Normaliza a [-1, 1]
        if (fabs(senal->muestras[i]) > max_valor) {
            max_valor = fabs(senal->muestras[i]);
        }
        
        // Filtro pasa-bajos para reducir el ruido demasiado agudo
        if (i > 0) {
            senal->muestras[i] = 0.6 * senal->muestras[i] + 0.4 * senal->muestras[i-1];
        }
    }
    
    free(datos_raw); // Libera memoria de los datos crudos obtenidos
    
    // Normaliza la señal para que el máximo sea 1.0
    if (max_valor > 0) {
        for (int i = 0; i < num_muestras; i++) {
            senal->muestras[i] /= max_valor;
        }
    }
    
    // Verifica si hay suficiente energía en la señal, es decir, si no es silencio
    double energia = 0;
    for (int i = 0; i < num_muestras; i++) {
        energia += senal->muestras[i] * senal->muestras[i];
    }
    energia /= num_muestras;
    
    // Si la energía es muy baja o debil, se considera silencio
    if (energia < UMBRAL_SILENCIO) {
        printf("Se detecto una señal demasiado debil\n");
        free(senal->muestras);
        free(senal);
        return NULL;
    }
    
    return senal;
}

// Analiza la voz y detecta la frecuencia principal
void analizar_voz(SenalVoz *senal) {
    int num_muestras = senal->num_muestras;
    int num_frecuencias = (2000 * num_muestras) / FRECUENCIA_MUESTREO;
    if (num_frecuencias > num_muestras / 2) {
        num_frecuencias = num_muestras / 2;
    }
    
    double *magnitudes = malloc(num_frecuencias * sizeof(double));
    double max_magnitud = 0;
    double energia_total = 0;
    
    // Calcula la DFT (Transformada Discreta de Fourier)
    for (int k = 1; k < num_frecuencias; k++) {
        double parte_real = 0, parte_imag = 0;
        
        // Calcula la suma para la frecuencia k
        for (int n = 0; n < num_muestras; n++) {
            double ventana = 0.5 * (1 - cos(2 * PI * n / (num_muestras - 1))); // Ventana de Hann
            double angulo = -2 * PI * k * n / num_muestras;
            
            parte_real += senal->muestras[n] * ventana * cos(angulo);
            parte_imag += senal->muestras[n] * ventana * sin(angulo);
        }
        
        magnitudes[k] = sqrt(parte_real * parte_real + parte_imag * parte_imag);
        energia_total += magnitudes[k];
        
        if (magnitudes[k] > max_magnitud) {
            max_magnitud = magnitudes[k];
        }
    }
    
    // Calcula umbral dinámico (ignora frecuencias con poca energía)
    double umbral = max_magnitud * 0.1;
    
    // Busca la frecuencia principal (entre 80Hz y 300Hz)
    double frecuencia_principal = 0;
    double max_principal = 0;
    int num_picos = 0;
    
    for (int k = (80 * num_muestras) / FRECUENCIA_MUESTREO; k < (300 * num_muestras) / FRECUENCIA_MUESTREO; k++) {
        if (magnitudes[k] > umbral) {
            num_picos++;
            if (magnitudes[k] > max_principal) {
                max_principal = magnitudes[k];
                frecuencia_principal = (double)k * FRECUENCIA_MUESTREO / num_muestras;
            }
        }
    }
    
    // Guarda los resultados en archivos CSV
    guardar_muestras(senal, "muestras_audio.csv");
    guardar_resultados_dft(magnitudes, num_frecuencias, frecuencia_principal, "analisis_dft.csv");
    
    // Muestra resultados en pantalla
    printf("\nResultados del analisis\n");
    printf("Frecuencia principal: %.0f Hz\n", frecuencia_principal);
    
    if (frecuencia_principal < 110) printf("Voz muy grave (hombre bajo)\n");
    else if (frecuencia_principal < 150) printf("Voz grave (hombre promedio)\n");
    else if (frecuencia_principal < 180) printf("Voz media (unisex)\n");
    else if (frecuencia_principal < 220) printf("Voz aguda (mujer promedio)\n");
    else printf("Voz muy aguda (como niño o mujer alta)\n");
    
    free(magnitudes);
}

int main() {
    printf("\nProcesador de audio (voz)\n\n");
    printf("A continuacion se grabaran 3 segundos de audio \nse procedera a analizar el tipo de voz mediante la DFT.\n");
    
    SenalVoz *senal = grabar_voz();
    if (!senal) {
        printf("\n\nALERTA: No se reconocio audio. Se usaran muestras de voz de ejemplo \n");
        senal = generar_voz_ejemplo();
    }
    
    if (senal) {
        analizar_voz(senal);
        free(senal->muestras);
        free(senal);
    } else {
        printf("\n\nError: No se pudo analizar la voz.\n");
    }
    
    printf("\nCerrando programa\n");
    return 0;
}