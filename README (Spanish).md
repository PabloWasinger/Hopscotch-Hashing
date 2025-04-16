# Hopscotch Hashing Implementation in C

Este proyecto implementa una tabla hash utilizando el algoritmo Hopscotch Hashing en C. Esta estructura de datos proporciona operaciones de diccionario eficientes (inserción, búsqueda y eliminación) con un rendimiento de tiempo constante O(1).

## ¿Qué es Hopscotch Hashing?

Hopscotch Hashing es un algoritmo de tabla hash desarrollado por Maurice Herlihy, Nir Shavit y Moran Tzafrir en 2008. Combina las ventajas de hashing abierto y cerrado, y está diseñado para tener un buen rendimiento incluso con altos factores de carga.

### Características principales:

- **Neighborhood chaining**: Cada clave se almacena en una posición dentro de su "vecindario", que es un rango de posiciones cercanas a su ubicación hash original.
- **Acceso de baja latencia**: Garantiza que todas las claves se encuentren cerca de su posición hash ideal.
- **Alta utilización del espacio**: Funciona bien incluso cuando la tabla está casi llena (factor de carga de 90%).
- **Rendimiento predecible**: Evita la degradación extrema que ocurre en otras implementaciones de hash cuando aumenta el factor de carga.

## Estructura de datos

La implementación utiliza las siguientes estructuras:

```c
typedef struct Element {
  const char *key;
  void *value;
  size_t hashvalue;
} Element;

typedef struct dictionary {
  Element *elements;     // Arreglo de elementos
  size_t size;           // Número de elementos almacenados
  size_t capacity;       // Capacidad total del arreglo
  destroy_f destroy;     // Función para liberar los valores
  uint32_t *bitmap;      // Mapa de bits para rastrear posiciones ocupadas
} dictionary_t;
```

## Parámetros de configuración

```c
#define INITIAL_CAPACITY 1024   // Capacidad inicial de la tabla
#define HOP_RANGE 32            // Tamaño máximo del vecindario
#define BIT_RANGE 0xFFFFFFFF    // Máscara de bits para el bitmap
#define SEED 42                 // Semilla para la función hash
```

## Algoritmo

### Funciones principales

1. **Inserción (`dictionary_put`)**:
   - Calcula el hash de la clave
   - Verifica si la clave ya existe
   - Si existe espacio en el vecindario, inserta directamente
   - Si no, desplaza elementos existentes para hacer espacio
   - Si es necesario, realiza rehashing para ampliar la tabla

2. **Búsqueda (`dictionary_get`)**:
   - Calcula el hash de la clave
   - Busca la clave en su vecindario utilizando el bitmap
   - Retorna el valor asociado o NULL si no se encuentra

3. **Eliminación (`dictionary_delete`, `dictionary_pop`)**:
   - Localiza la clave en su vecindario
   - Elimina el elemento y actualiza el bitmap
   - Retorna el valor o lo destruye según la operación

### Manejo de colisiones

La estrategia de Hopscotch Hashing para manejar colisiones incluye:

1. **Búsqueda de espacio libre dentro del vecindario**: Cada clave debe almacenarse dentro del vecindario de su posición hash original.

2. **Desplazamiento de elementos** (`displace_keys`): Cuando un vecindario está lleno, los elementos existentes se reordenan para hacer espacio para la nueva clave.

3. **Rehashing** (`rehash`): Si no es posible reordenar los elementos, la tabla se redimensiona duplicando su capacidad y todos los elementos se redistribuyen.

### Optimizaciones

- **Uso de bitmaps**: Los vecindarios se gestionan mediante mapas de bits, lo que reduce los accesos a memoria.
- **Prefetching**: Uso de `__builtin_prefetch` para mejorar el rendimiento de caché.
- **Operaciones de bits eficientes**: Uso de técnicas como `__builtin_clz` para operaciones rápidas sobre los bitmaps.
- **Función hash Murmur**: Proporciona una distribución de alta calidad con buenas propiedades de avalancha.

## API del diccionario

```c
// Crea un nuevo diccionario con una función opcional de destrucción
dictionary_t *dictionary_create(destroy_f destroy);

// Inserta o actualiza un par clave-valor
bool dictionary_put(dictionary_t *dictionary, const char *key, void *value);

// Obtiene un valor a partir de su clave
void *dictionary_get(dictionary_t *dictionary, const char *key, bool *err);

// Elimina una clave y su valor asociado
bool dictionary_delete(dictionary_t *dictionary, const char *key);

// Elimina y retorna un valor sin destruirlo
void *dictionary_pop(dictionary_t *dictionary, const char *key, bool *err);

// Verifica si una clave existe
bool dictionary_contains(dictionary_t *dictionary, const char *key);

// Retorna el número de elementos
size_t dictionary_size(dictionary_t *dictionary);

// Libera el diccionario y sus elementos
void dictionary_destroy(dictionary_t *dictionary);
```

## Ejemplos de uso

### Ejemplo básico

```c
#include "hopscotch_hash.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Crear un diccionario que libere valores con free
    dictionary_t *dict = dictionary_create(free);
    
    // Insertar algunos pares clave-valor
    char *value1 = strdup("valor1");
    char *value2 = strdup("valor2");
    
    dictionary_put(dict, "clave1", value1);
    dictionary_put(dict, "clave2", value2);
    
    // Buscar un valor
    bool err;
    char *result = dictionary_get(dict, "clave1", &err);
    if (!err) {
        printf("Valor para clave1: %s\n", result);
    }
    
    // Verificar existencia
    if (dictionary_contains(dict, "clave2")) {
        printf("clave2 existe en el diccionario\n");
    }
    
    // Liberar el diccionario y todos sus valores
    dictionary_destroy(dict);
    
    return 0;
}
```

## Rendimiento

La implementación de Hopscotch Hashing ofrece:

- Operaciones O(1) en condiciones normales
- Alto rendimiento incluso con factores de carga del 90%
- Escalabilidad predecible a medida que aumenta el número de elementos

Los tests de rendimiento (`tests.c`) incluyen:
- Pruebas con hasta 1,048,576 elementos
- Pruebas de inserción y eliminación aleatorias
- Verificación de coherencia después de muchas operaciones

## Referencias

- Herlihy, M., Shavit, N., & Tzafrir, M. (2008). "Hopscotch Hashing". DISC '08: Proceedings of the 22nd international symposium on Distributed Computing.
- Celis, P. (1986). "Robin Hood Hashing". PhD Thesis, University of Waterloo.

## Licencia

Este código se distribuye bajo licencia MIT.
