# 🖼️ Crossfading entre imágenes (Secuencial, OpenMP y MPI)

Este proyecto implementa un **efecto de transición (cross-fading)** entre una imagen en color y su versión en escala de grises, generando una secuencia de frames y luego un video.  

![imagen_color](./imagen_color_800x800.png)!
[imagen_blanco/negro](./imagen_b_n.png)

Incluye **tres versiones en C**:
- ✅ **Implementación secuencial**
- ✅ **Implementación paralela usando OpenMP**
- ✅ **Implementación distribuida usando MPI**

El objetivo es comparar rendimiento, analizar escalabilidad y aplicar técnicas de paralelismo tanto a nivel de CPU (OpenMP) como de clúster (MPI).

---

## 📌 Objetivos

- Procesar una imagen RGB y convertirla a escala de grises
- Generar **96 frames** aplicando interpolación lineal:  
  ```
  result = color × P + gris × (1 - P)
  ```
- Implementar el mismo algoritmo en tres versiones:
  - **Secuencial**
  - **OpenMP** → Paralelismo de memoria compartida
  - **MPI** → Procesamiento distribuido por filas
- Medir tiempo de procesamiento **excluyendo operaciones lentas de I/O**
- Construir un video final con **FFmpeg**

---

## 📁 Estructura del proyecto

```
/
├── crossfade_secuencial.c     # Implementación secuencial
├── crossfade_openmp.c         # Implementación paralela con OpenMP
├── crossfade_mpi.c            # Implementación distribuida con MPI
├── imagen_color_800x800.png   # Imagen de entrada (RGB)
├── imagen_color_2000x2000.png   # Imagen opcional 2 (RGB)
├── imagen_color_5000x5000.png   # Imagen opcional 3 (RGB)
├── stb_image.h                # Librería para cargar imágenes
├── stb_image_write.h          # Librería para guardar imágenes
├── README.md                  # Documentación del proyecto
```

> **Nota:** Los archivos `stb_image.h` y `stb_image_write.h` son librerías de dominio público utilizadas para leer y escribir PNG.

---

## ⚙️ Requisitos previos

- **Compilador C**: GCC o MinGW
- **OpenMP**: Para la versión paralela multinúcleo
- **MPI**: MPICH para la versión distribuida
- **FFmpeg** *(opcional)*: Para generar el video final


## 🚀 Compilación y ejecución

### 1️⃣ Versión Secuencial

**Compilar:**
```bash
gcc crossfade_secuencial.c -o crossfade_secuencial -fopenmp -lm
```

**Ejecutar:**
```bash
./crossfade_secuencial
```

---

### 2️⃣ Versión OpenMP

**Compilar:**
```bash
gcc crossfade_openmp.c -o crossfade_openmp -fopenmp -lm
```

**Ejecutar:**
```bash
./crossfade_openmp
```

**Configurar número de hilos:**
```bash
# Windows
set OMP_NUM_THREADS=8

# Linux/MSYS2
export OMP_NUM_THREADS=8
```

---

### 3️⃣ Versión MPI

**Compilar:**
```bash
mpicc crossfade_mpi.c -o crossfade_mpi -lm
```

**Ejecutar con N procesos:**
```bash
mpiexec -n 4 ./crossfade_mpi.exe
```

> **Tip:** Ajusta el número de procesos según los cores disponibles en tu sistema.

---

## 🎬 Generar video MP4 con FFmpeg

Una vez generados los frames (`frame_000.png` a `frame_095.png`):

```bash
ffmpeg -framerate 24 -i frame_%03d.png -c:v libx264 -pix_fmt yuv420p crossfade_output.mp4
```

---

## 📈 Resultados experimentales

| Configuración | Tiempo (segundos) | Speedup |
|---------------|-------------------|---------|
| Secuencial | 3.2923 s | 1.00× |
| OpenMP (8 hilos) | 0.5948 s | 5.5351× |
| MPI (8 procesos) | 0.5287 s | 6.2271× |

> *Datos obtenidos con imagen de 2000×2000 píxeles en CPU Ryzen 7 5700u, 8 núcleos / 16 hilos, 40 GB de RAM, sistema operativo Linux Ubuntu 25.04.*

---

## 📩 Contacto

Si deseas sugerir mejoras o reportar errores:
- 🐛 Abre un **Issue** en este repositorio
- 🔧 Envía un **Pull Request** con tus mejoras
- 📧 Contactame por email

---

## 📄 Licencia

Este proyecto es de código abierto y está disponible bajo la licencia MIT.

---

**⭐ Si te resultó útil este proyecto, dale una estrella en GitHub!**