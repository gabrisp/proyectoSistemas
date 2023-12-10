#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_LINEAS 8
#define TAM_LINEA 16
#define NUM_FILAS 8

typedef struct {
    unsigned char ETQ;
    unsigned char Data[TAM_LINEA];
} T_CACHE_LINE;

unsigned char Simul_RAM[4096];
int globaltime = 0;
int numfallos = 0;


void LimpiarCACHE(T_CACHE_LINE tbl[NUM_FILAS]);
void VolcarCACHE(T_CACHE_LINE *tbl);
void ParsearDireccion(unsigned int addr, int *ETQ, int *palabra, int *linea, int *bloque);
void TratarFallo(T_CACHE_LINE *tbl, unsigned char *MRAM, int ETQ, int linea, int bloque);

int main() {
    FILE *ram_file = fopen("CONTENTS_RAM.bin", "rb");
    FILE *mem_file = fopen("accesos_memoria.txt", "r");

    if (ram_file == NULL || mem_file == NULL) {
        perror("Error al abrir fichero");
        return -1;
    }

    // Inicializar Simul_RAM y tbl
    fread(Simul_RAM, sizeof(unsigned char), sizeof(Simul_RAM), ram_file); // Leemos ram
    T_CACHE_LINE tbl[NUM_FILAS];
    LimpiarCACHE(tbl); // Inicializamos

    unsigned int addr;
    char texto[100];

    while (fscanf(mem_file, "%x", &addr) != EOF) {
        int ETQ, palabra, linea, bloque;
        ParsearDireccion(addr, &ETQ, &palabra, &linea, &bloque);

         if (tbl[linea].ETQ != ETQ) {
            numfallos++;
            printf("T: %d, Fallo de CACHE %d, ADDR %04X Label %X linea %02X palabra %02X bloque %02X\n",
                   globaltime, numfallos, addr, tbl[linea].ETQ, linea, palabra, bloque);

            TratarFallo(tbl, Simul_RAM, ETQ, linea, bloque);

            printf("Cargando bloque %02X en la línea %02X\n", bloque, linea);

            // Se incrementa globaltime en 20
            globaltime += 20;
        } else {
            printf("T: %d, Acierto de CACHE, ADDR %04X Label %X linea %02X palabra %02X DATO %02X\n",
                   globaltime, addr, tbl[linea].ETQ, linea, palabra, tbl[linea].Data[palabra]);
            int j = (globaltime++)-(numfallos * 20);
            texto[j] = tbl[linea].Data[palabra];
        }
        VolcarCACHE(tbl);
        sleep(1);
    }

    fclose(ram_file);
    fclose(mem_file);
    int aciertos = globaltime-(numfallos * 20);
    printf("\nAccesos totales: %d; fallos: %d; tiempo medio: %.2fs\n",
           aciertos+numfallos, numfallos, (float)globaltime/ (aciertos + numfallos));
    printf("Texto leído: %s\n", texto);


    // Volcar el contenido de la caché en el archivo binario CONTENTS_CACHE.bin
    FILE *cache_file = fopen("CONTENTS_CACHE.bin", "wb");
    fwrite(tbl, sizeof(T_CACHE_LINE), NUM_FILAS, cache_file);
    fclose(cache_file);

    return 0;
}

void LimpiarCACHE(T_CACHE_LINE tbl[NUM_FILAS]) {
    for (int i = 0; i < NUM_FILAS; i++) {
        tbl[i].ETQ = 0xFF;  // Inicializa la etiqueta con 0xFF
        for (int j = 0; j < TAM_LINEA; j++) {
            tbl[i].Data[j] = 0x23;  // Inicializa los datos con 0x23
        }
    }
}

void ParsearDireccion(unsigned int addr, int *ETQ, int *palabra, int *linea, int *bloque) {
    *palabra = addr & 0xF;          // 4 bits menos significativos para la palabra
    *linea = (addr >> 4) & 0x7;     // 3 siguientes bits para la línea
    *bloque = addr >> 7;            // resto de bits para el bloque
    *ETQ = *bloque & 0x1F;           // 5 bits menos significativos del bloque para la etiqueta
}



void VolcarCACHE(T_CACHE_LINE *tbl) {
    for (int i = 0; i < NUM_FILAS; i++) {
        printf("%02X Datos: ", tbl[i].ETQ);
        for (int j = TAM_LINEA - 1; j >= 0; j--) {
            printf("%02X ", tbl[i].Data[j]);
        }
        printf("\n");
    }
    printf("\n");
}


void TratarFallo(T_CACHE_LINE *tbl, unsigned char *MRAM, int ETQ, int linea, int bloque) {
    
    // Obtener la posición en Simul_RAM del bloque que se va a cargar en caché
    int pos_RAM = bloque * TAM_LINEA;

    // Actualizar la línea de la caché
    tbl[linea].ETQ = ETQ;
    for (int i = 0; i < TAM_LINEA; i++) {
        tbl[linea].Data[i] = MRAM[pos_RAM + i];
    }
}
