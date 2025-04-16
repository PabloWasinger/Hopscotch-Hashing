#include "tp3.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>



#define INITIAL_CAPACITY 1024
#define HOP_RANGE 32
#define BIT_RANGE 0xFFFFFFFF
#define SEED 42
#define ROTL32(x, r) ((x << r) | (x >> (32 - r)))

bool dictionary_put_with_hashvalue(dictionary_t *dictionary, const char *key, void *value, size_t hash_value);
size_t get_keys_matching_index_in_neighborhood(dictionary_t *dictionary, const char *key, size_t pos);

typedef struct Element{
  const char *key;
  void *value;
  size_t hashvalue;
}Element;


typedef struct dictionary {
  Element *elements;
  size_t size;
  size_t capacity;
  destroy_f destroy;
  uint32_t *bitmap;
}dictionary_t;

const char *my_strdup(const char *src) {
    if (src == NULL) return NULL;

    size_t len = strlen(src) + 1;
    char *dest = malloc(len);
    if (dest == NULL) {
        return NULL;
    }

    memcpy(dest, src, len);
    return dest;
}




size_t murmur_hash(const char *key, int seed) {
    int len = (int) (strlen(key) + 1);
    const uint8_t *data = (const uint8_t *)key;
    const int nblocks = len / 4;

    uint32_t h1 = seed;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const uint32_t *blocks = (const uint32_t *)(data + nblocks * 4);
    for (int i = -nblocks; i; i++) {
        __builtin_prefetch(&blocks[i - 1], 0, 1);
        uint32_t k1 = blocks[i];

        k1 *= c1;
        k1 = ROTL32(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1 = ROTL32(h1, 13);
        h1 = h1 * 5 + 0xe6546b64;
    }

    const uint8_t *tail = (const uint8_t *)(data + nblocks * 4);
    uint32_t k1 = 0;

    switch (len & 3) {
        case 3: k1 ^= tail[2] << 16;
        case 2: k1 ^= tail[1] << 8;
        case 1: k1 ^= tail[0];
                k1 *= c1;
                k1 = ROTL32(k1, 15);
                k1 *= c2;
                h1 ^= k1;
    }

    // Finalization
    h1 ^= len;
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    return h1;
}



bool neighborhood_is_full(size_t bitmap) {
    return bitmap == BIT_RANGE;
}


bool update_bitmap(uint32_t *bitmap, size_t position, size_t neighbor, bool update) {
    if (!bitmap) return false;
    uint32_t change = (1 << (31 - neighbor)); // Desplaza para que `neighbor` sea desde la izquierda

    if (update) bitmap[position] |= change; 

    else bitmap[position] &= ~change;
    
    return true;
}


int get_first_zero_position(uint32_t bitmap) {
    uint32_t inverted = ~bitmap;
    if (inverted == 0) {
        return -1; // No hay bits libres
    }
    int position = __builtin_clz(inverted); // Cuenta los ceros a la izquierda
    return position;
}



size_t return_free_index_in_neighborhood(dictionary_t *dictionary, size_t pos) {
    uint32_t bitmap = dictionary->bitmap[pos];
    uint32_t inverted = ~bitmap;

    if (inverted == 0) {
        return dictionary->capacity; // No hay espacios libres en el vecindario
    }

    while (inverted != 0) {
        uint32_t first_free = inverted & -inverted;
        if (first_free == 0) {
            return dictionary->capacity; // No se encontró espacio libre
        }
        size_t neighbor = __builtin_clz(first_free);

        size_t index = (pos + neighbor) % dictionary->capacity;

        
        if (dictionary->elements[index].key == NULL) {
            return neighbor;
        }
        inverted &= ~first_free;
    }

    return dictionary->capacity; // No se encontró espacio libre en el vecindario
}


size_t find_first_free_element(dictionary_t *dictionary, size_t pos){
    size_t capacity = dictionary->capacity;
    for (size_t i = HOP_RANGE; i < capacity; i++){
        size_t index = (pos + i) % capacity;
        if (!dictionary->elements[index].key) return index;
    }
    return capacity; // Indica que no se encontró una posición libre
}




size_t displace_keys(dictionary_t *dictionary, size_t pos){
    size_t capacity = dictionary->capacity;
    size_t current_free = find_first_free_element(dictionary, pos);
    if (current_free >= capacity) return capacity; // No se encontró posición libre

    while ((current_free + capacity - pos) % capacity >= HOP_RANGE){
        int found = 0;
        
        for (int i = HOP_RANGE - 1; i >= 0; i--){
            size_t idx = (current_free + capacity - i) % capacity;
            Element *elem = &dictionary->elements[idx];
            

            __builtin_prefetch(&dictionary->elements[(current_free + capacity - (i - 1)) % capacity], 0, 1);

            if (elem->key == NULL) continue;
            size_t elem_bucket = elem->hashvalue % capacity;
            if ((current_free + capacity - elem_bucket) % capacity < HOP_RANGE){
                
                // Mover elemento de idx a current_free
                dictionary->elements[current_free] = *elem;
                elem->key = NULL; // Marcar idx como libre
                elem->value = NULL;

                // Actualizar el bitmap del bucket original
                size_t old_offset = (idx + capacity - elem_bucket) % capacity;
                size_t new_offset = (current_free + capacity - elem_bucket) % capacity;
                dictionary->bitmap[elem_bucket] &= ~(1U << old_offset);
                dictionary->bitmap[elem_bucket] |= (1U << new_offset);

                current_free = idx;
                found = 1;
                break;
            }
        }
        if (!found){
            return capacity; // No se pudo desplazar
        }
    }
    return current_free; // Éxito, posición libre dentro del vecindario
}


bool insert_element(dictionary_t *dictionary, size_t pos, size_t hashvalue, void* value, const char* key){
    if (!dictionary || pos >= dictionary->capacity) return false;


    __builtin_prefetch(&dictionary->elements[pos], 0, 1);

    size_t bucket_index = hashvalue % dictionary->capacity;
    size_t neighbor = (pos + dictionary->capacity - bucket_index) % dictionary->capacity;

    if (neighbor >= HOP_RANGE) return false;

    const char* dup_key = my_strdup(key);
    if (!dup_key) {
        return false; 
    }

    if (!update_bitmap(dictionary->bitmap, bucket_index, neighbor, true)) {

        free((char *)dup_key);
        return false;
    }

    dictionary->elements[pos].key = dup_key;
    dictionary->elements[pos].hashvalue = hashvalue;
    dictionary->elements[pos].value = value;

    dictionary->size++;

    return true;
}





bool rehash(dictionary_t *dictionary){
   Element *old_elements = dictionary->elements;
    uint32_t *old_bitmap = dictionary->bitmap;
    size_t old_capacity = dictionary->capacity;

    dictionary->capacity = old_capacity * 2;
    dictionary->size = 0; 

    dictionary->elements = calloc(dictionary->capacity, sizeof(Element));
    if (!dictionary->elements) {
        dictionary->elements = old_elements;
        dictionary->capacity = old_capacity;
        return false;
    }

    dictionary->bitmap = calloc(dictionary->capacity, sizeof(uint32_t));
    if (!dictionary->bitmap) {
        free(dictionary->elements);
        dictionary->elements = old_elements;
        dictionary->bitmap = old_bitmap;
        dictionary->capacity = old_capacity;
        return false;
    }

    for (size_t i = 0; i < old_capacity; i++) {
        if (old_elements[i].key != NULL) {
            if (!dictionary_put_with_hashvalue(dictionary, old_elements[i].key, old_elements[i].value, old_elements[i].hashvalue)) {

                free(dictionary->elements);
                free(dictionary->bitmap);
                dictionary->elements = old_elements;
                dictionary->bitmap = old_bitmap;
                dictionary->capacity = old_capacity;
                return false;
            }
        }
    }

    for (size_t i = 0; i < old_capacity; i++) {
        if (old_elements[i].key != NULL) {
            free((void *)old_elements[i].key);
        }
    }

    free(old_elements);
    free(old_bitmap);
    return true;
}




dictionary_t *dictionary_create(destroy_f destroy) {
  dictionary_t *dic = malloc(sizeof(dictionary_t));
  if (!dic) {
    return NULL;
  }

  dic->capacity = INITIAL_CAPACITY; //Con cuanto hay que empezar?
  dic->size = 0;
  dic->destroy = destroy;
  dic->elements = calloc(INITIAL_CAPACITY, sizeof(Element));
  if (!dic->elements) {
    free(dic);
    return NULL;
  }
  dic->bitmap = calloc(INITIAL_CAPACITY, sizeof(uint32_t));
    if (!dic->bitmap) {
        free(dic->elements);
        free(dic);
        return NULL;
    }

  return dic;
}


bool dictionary_put_with_hashvalue(dictionary_t *dictionary, const char *key, void *value, size_t hash_value){
    if (!dictionary) return false;

    size_t capacity = dictionary->capacity;
    size_t pos = hash_value % capacity;

    size_t existing_index = get_keys_matching_index_in_neighborhood(dictionary, key, pos);
    if (existing_index != dictionary->capacity){
        if (dictionary->destroy && dictionary->elements[existing_index].value != NULL) {
            dictionary->destroy(dictionary->elements[existing_index].value);
        }
        dictionary->elements[existing_index]. value = value;
        return true;
    }

    size_t free_element = return_free_index_in_neighborhood(dictionary, pos);
    if (free_element != capacity){
        if (!insert_element(dictionary, (pos + free_element) % capacity, hash_value, value, key)) {
            return false;
        }
        return true;
    }

    free_element = displace_keys(dictionary, pos);
    if (free_element != capacity){
        if (!insert_element(dictionary, free_element, hash_value, value, key)) {
            return false;
        }
        return true;
    }

    if (!rehash(dictionary)){
        return false;
    }

    return dictionary_put_with_hashvalue(dictionary, key, value, hash_value);
}



bool dictionary_put(dictionary_t *dictionary, const char *key, void *value) {
  return dictionary_put_with_hashvalue(dictionary, key, value, murmur_hash(key, SEED));
}


size_t get_keys_matching_index_in_neighborhood(dictionary_t *dictionary, const char *key, size_t pos){

  uint32_t bitmap = dictionary->bitmap[pos];
  uint32_t first_occupied;  // Encuentra el primer bit 1
  size_t neighbor;
  size_t index;
  while (bitmap != 0) {
        first_occupied = bitmap & -bitmap;
        if (first_occupied == 0) {
            return dictionary->capacity; // No se encontró un vecino ocupado
        }
        neighbor = __builtin_clz(first_occupied);
        index = (pos + neighbor) % dictionary->capacity;
        if (dictionary->elements[index].key != NULL) {
            if (strcmp(key, dictionary->elements[index].key) == 0){
              return index;
            }
        }
        bitmap &= ~first_occupied;
    }

  return dictionary->capacity;
}


void *dictionary_get(dictionary_t *dictionary, const char *key, bool *err) {
  
  size_t pos = murmur_hash(key, SEED) % dictionary->capacity;
  size_t index = get_keys_matching_index_in_neighborhood(dictionary, key, pos);
  if (index == dictionary->capacity){
    *err = true;
    return NULL;
  }

  *err = false;
  return dictionary->elements[index].value;

};




bool dictionary_delete(dictionary_t *dictionary, const char *key) {
  
  bool err = false;
  void *value = dictionary_pop(dictionary, key, &err);
  if (err) return false;
  if (dictionary->destroy != NULL && value != NULL){
    dictionary->destroy(value);
  }
  return true;
}




void *dictionary_pop(dictionary_t *dictionary, const char *key, bool *err) {
  
  size_t pos = murmur_hash(key, SEED) % dictionary->capacity;
  size_t index = get_keys_matching_index_in_neighborhood(dictionary, key, pos);
  if (index == dictionary->capacity){
    *err = true;
    return NULL;
  }

  void *value = dictionary->elements[index].value;
  free((void *)dictionary->elements[index].key);
  dictionary->elements[index].key = NULL;
  dictionary->elements[index].value = NULL;

  size_t neighbor = (index + dictionary->capacity - pos) % dictionary->capacity;
  update_bitmap(dictionary->bitmap, pos, neighbor, false);

  dictionary->size--;

  *err = false;
  return value;
};




bool dictionary_contains(dictionary_t *dictionary, const char *key) {
  size_t pos = murmur_hash(key, SEED) % dictionary->capacity;
  size_t index = get_keys_matching_index_in_neighborhood(dictionary, key, pos);
  if (index == dictionary->capacity){
    return false;
  }

  return true;
};




size_t dictionary_size(dictionary_t *dictionary) { return dictionary->size; };




void dictionary_destroy(dictionary_t *dictionary){

for (size_t i = 0; i < dictionary->capacity; i++) {
    if (dictionary->elements[i].key != NULL) {
        if (dictionary->destroy) {
            dictionary->destroy(dictionary->elements[i].value);
        }
        free((void *)dictionary->elements[i].key);
    }
}

  free(dictionary->elements);
  free(dictionary->bitmap);
  free(dictionary);
  return;
};
